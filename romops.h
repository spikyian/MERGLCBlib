#ifndef _ROMOPS_H_
#define _ROMOPS_H_

// NVM types
typedef enum {
    EEPROM_NVM_TYPE,
    FLASH_NVM_TYPE
} NVMtype;

#ifdef EE256
    #define EE_TOP  0xFF
    #define EE_BOTTOM 0x00
    #define	SET_EADDRH(val)             	// EEPROM high address reg not present in 2480/2580, so just do nothing
#endif

#ifdef _PIC18
    #define EE_TOP  0x3FF
    #define EE_BOTTOM 0x00
    #define	SET_EADDRH(val) EEADRH = val	// EEPROM high address is present, so write value
#endif


    // Call back into the application to check if now is a good time to write the flash
    // as the processor will be suspended for up to 2ms.
extern unsigned char APP_isSuitableTimeToWriteFlash(void);

void initRomOps(void);
int16_t readNVM(NVMtype type, uint24_t index);
uint8_t writeNVM(NVMtype type, uint24_t index, uint8_t value);

#endif