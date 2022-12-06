#ifndef _ROMOPS_H_
/*
  This work is licensed under the:
      Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
   To view a copy of this license, visit:
      http://creativecommons.org/licenses/by-nc-sa/4.0/
   or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

   License summary:
    You are free to:
      Share, copy and redistribute the material in any medium or format
      Adapt, remix, transform, and build upon the material

    The licensor cannot revoke these freedoms as long as you follow the license terms.

    Attribution : You must give appropriate credit, provide a link to the license,
                   and indicate if changes were made. You may do so in any reasonable manner,
                   but not in any way that suggests the licensor endorses you or your use.

    NonCommercial : You may not use the material for commercial purposes. **(see note below)

    ShareAlike : If you remix, transform, or build upon the material, you must distribute
                  your contributions under the same license as the original.

    No additional restrictions : You may not apply legal terms or technological measures that
                                  legally restrict others from doing anything the license permits.

   ** For commercial use, please contact the original copyright holder(s) to agree licensing terms

    This software is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE

  Ian Hogg Nov 2022
 */
/*
 * Romops provides functions for storing/retrieving data from EEPROM and Flash 
 * memory.
 */
#define _ROMOPS_H_

// NVM types
typedef enum {
    EEPROM_NVM_TYPE,
    FLASH_NVM_TYPE
} NVMtype;

typedef enum ValidTime {
    BAD_TIME=0,
    GOOD_TIME=1
} ValidTime;

/*
 * Processor specific settings
 */
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


/**
 * Call back into the application to check if now is a good time to write the flash
 * as the processor will be suspended for up to 2ms.
 */
extern ValidTime APP_isSuitableTimeToWriteFlash(void);

/**
 * Initialise the Romops functions. Sets the flash buffer as being currently unused. 
 */
void initRomOps(void);

/**
 * Read a byte from NVM.
 * @param type specify the type of NVM required
 * @param index is the address to be read
 * @return the byte read from NMS or, if negative, the error
 */
int16_t readNVM(NVMtype type, uint24_t index);

/**
 * Write a byte to NVM.
 * @param type specify the type of NVM required
 * @param index is the address to be written
 * @param value the byte value to be written
 * @return 0 for success or error number
 */
uint8_t writeNVM(NVMtype type, uint24_t index, uint8_t value);

#endif