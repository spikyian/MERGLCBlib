#include <xc.h>
#include "merglcb.h"
#include "hardware.h"
#include "romops.h"
/////////////////////////////////////////////
// NON VOLATILE MEMORY FUNCTIONS
/////////////////////////////////////////////
#ifdef _PIC18
#define BLOCK_SIZE _FLASH_ERASE_SIZE
#endif

// Structure for tracking Flash operations

static union
{
    uint8_t asByte;       
    struct  {
        uint8_t writeNeeded:1; //flag if buffer is modified
        uint8_t eraseNeeded:1;  //flag if long write with block erase
    };
} flashFlags;
static uint8_t     flashBuffer[BLOCK_SIZE];    // Assumes that Erase and Write are the same size
static uint24_t    flashBlock;     //address of current 64 byte flash block

#define BLOCK(A)    (A&(uint24_t)(~(BLOCK_SIZE-1)))
#define OFFSET(A)   (A&(BLOCK_SIZE-1))


int16_t read_eeprom(uint16_t index) {
    // do read of EEPROM
    while (EECON1bits.WR)       // Errata says this is required
        ;
    // EEADRH = index >> 8;        //  High byte of address to read
    SET_EADDRH((index >> 8)&0xFF);
    EEADR = index & 0xFF;       	/* Low byte of Data Memory Address to read */
    EECON1bits.EEPGD = 0;    	/* Point to DATA memory */
    EECON1bits.CFGS = 0;    	/* Access program FLASH/Data EEPROM memory */
    EECON1bits.RD = 1;			/* EEPROM Read */
    while (EECON1bits.RD)
        ;
#ifdef __XC8
    asm("NOP");                 /* data available after a NOP */
#else
    _asm
        nop
    _endasm
#endif
    return EEDATA;
}

uint8_t write_eeprom(uint16_t index, uint8_t value) {
    uint8_t interruptEnabled;
    interruptEnabled = geti(); // store current global interrupt state
    do {
        SET_EADDRH((index >> 8)&0xFF);      // High byte of address to write
        EEADR = index & 0xFF;       	/* Low byte of Data Memory Address to write */
        EEDATA = value;
        EECON1bits.EEPGD = 0;       /* Point to DATA memory */
        EECON1bits.CFGS = 0;        /* Access program FLASH/Data EEPROM memory */
        EECON1bits.WREN = 1;        /* Enable writes */
        di();                       /* Disable Interrupts */
        EECON2 = 0x55;
        EECON2 = 0xAA;
        EECON1bits.WR = 1;
        while (EECON1bits.WR)       // should wait until WR clears
            ;
        if (interruptEnabled) {     // Only enable interrupts if they were enabled at function entry
            ei();                   /* Enable Interrupts */
        }
        while (!EEIF)
            ;
        EEIF = 0;
        EECON1bits.WREN = 0;		/* Disable writes */
    } while (readNVM(EEPROM_NVM_TYPE, index) != value);
    return GRSP_OK;
}

int16_t read_flash(uint24_t index) {
    // do read of Flash
    if (BLOCK(index) == flashBlock) {
        // if the block is the current one then get it directly
        return flashBuffer[OFFSET(index)];
    } else {
        // we'll read single byte from flash
        TBLPTR = index;
        asm("TBLRD");
        return TABLAT;
    }
}

void eraseFlashBlock() {
    uint8_t interruptEnabled;
    // Call back into the application to check if now is a good time to write the flash
    // as the processor will be suspended for up to 2ms.
    while (! APP_isSuitableTimeToWriteFlash());
    
    interruptEnabled = geti(); // store current global interrupt state
    TBLPTR = flashBlock;
    EECON1bits.EEPGD = 1;   // 1=Program memory, 0=EEPROM
    EECON1bits.CFGS = 0;    // 0=Program memory/EEPROM, 1=ConfigBits
    EECON1bits.WREN = 1;    // enable write to memory
    EECON1bits.FREE = 1;    // enable row erase operation
    di();     // disable all interrupts
    EECON2 = 0x55;          // write 0x55
    EECON2 = 0xaa;          // write 0xaa
    EECON1bits.WR = 1;      // start erasing
    while(EECON1bits.WR)    // wait to finish
        ;
    if (interruptEnabled) {     // Only enable interrupts if they were enabled at function entry
        ei();                   /* Enable Interrupts */
    }
    EECON1bits.WREN = 0;    // disable write to memory
}

void flushFlashBlock() {
    uint8_t interruptEnabled;
    TBLPTR = flashBlock; //force row boundary
    interruptEnabled = geti(); // store current global interrupt state
    di();     // disable all interrupts ERRATA says this is needed before TBLWT
    for (unsigned char i=0; i<BLOCK_SIZE; i++) {
        TABLAT = flashBuffer[i];
        asm("TBLWT*+");
    }
    // Note from data sheet: 
    //   Before setting the WR bit, the Table
    //   Pointer address needs to be within the
    //   intended address range of the 64 bytes in
    //   the holding register.
    // So we put it back into the block here
    TBLPTR = flashBlock;
    EECON1bits.EEPGD = 1;   // 1=Program memory, 0=EEPROM
    EECON1bits.CFGS = 0;    // 0=ProgramMemory/EEPROM, 1=ConfigBits
    EECON1bits.FREE = 0;    // No erase
    EECON1bits.WREN = 1;    // enable write to memory
    
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    if (interruptEnabled) {     // Only enable interrupts if they were enabled at function entry
        ei();                   /* Enable Interrupts */
    }
    EECON1bits.WREN = 0;
}

void loadFlashBlock() {
    EECON1=0X80;    // access to flash
        TBLPTR = flashBlock;
        for (unsigned char i=0; i<64; i++) {
            asm("TBLRD*+");
            flashBuffer[i] = TABLAT;
        }
        TBLPTR = flashBlock;
}
    
uint8_t write_flash(uint24_t index, uint8_t value) {
    uint8_t oldValue;
    
    while (! APP_isSuitableTimeToWriteFlash())  // block awaiting a good time
        ;
    /*
     * Writing flash is a bit of a pain as you must write in blocks. If you want to
     * write just 1 byte then you need ensure you don't change any other byte in the
     * block. To do this the block must be read into a buffer the byte changed 
     * and then the whole buffer written. Unfortunately this algorithm would cause
     * too many writes to happen and the flash would wear out.
     * Instead after reading the block and updating the byte we don't write the
     * buffer back in case there is another update within the same block. The 
     * block is only written back if another block needs to be updated.
     * Whilst writing back if any bit changes from 0 to 1 then the block needs
     * to be erased before writing.
     *
     */
    if (BLOCK(index) == flashBlock) {
        // same block - just update the memory
        if (flashBuffer[OFFSET(index)] == value) {
            // No change
            return GRSP_OK;
        }
        flashFlags.eraseNeeded = (value & ~flashBuffer[OFFSET(index)])?1:0;
        flashBuffer[OFFSET(index)] = value;
        flashFlags.writeNeeded = 1;
    } else {
        // ok we want to write a different block so flush the current block 
        if (flashFlags.eraseNeeded) {
            eraseFlashBlock();
            flashFlags.eraseNeeded = 0;
        }
        
        if (flashFlags.writeNeeded) {
            flushFlashBlock();
            flashFlags.writeNeeded = 0;
        }
        // and load the new one
        flashBlock = BLOCK(index);
        loadFlashBlock();
        
        if (flashBuffer[OFFSET(index)] == value) {
            // No change
            return GRSP_OK;
        }
        flashFlags.eraseNeeded = (value & ~flashBuffer[OFFSET(index)])?1:0;
        flashBuffer[OFFSET(index)] = value;
        flashFlags.writeNeeded = 1;
    }
    return GRSP_OK;
}


uint8_t writeNVM(NVMtype type, uint24_t index, uint8_t value) {
    switch(type) {
        case EEPROM_NVM_TYPE:
            return write_eeprom((uint16_t)index, value);
        case FLASH_NVM_TYPE:
            return write_flash(index, value);
        default:
            return GRSP_UNKNOWN_NVM_TYPE;
    }
}
int16_t readNVM(NVMtype type, uint24_t index) {
    switch(type) {
        case EEPROM_NVM_TYPE:
            return read_eeprom((uint16_t)index);
        case FLASH_NVM_TYPE:
            return read_flash(index);
        default:
            return -GRSP_UNKNOWN_NVM_TYPE;
    }
}

/**
 *  Initialise variables for Flash program tracking.
 */
void initRomOps(void) {
    flashFlags.asByte = 0;
    flashBlock = 0x0000; // invalid but as long a write isn't needed it will be ok
    TBLPTRU = 0;
}

    
