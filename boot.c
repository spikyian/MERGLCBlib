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
#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "boot.h"
#include "mns.h"
#include "romops.h"

/*
 * Implementation of the BOOT service. Supports the FCU and CBUS (PIC based)
 * bootloading protocol.
 * In order to be compatible with the FCU bootloader there are additional
 * requirements for the parameter block which are actually supported by the MNS.
 */

// service decription
const Service bootService = {
    SERVICE_ID_BOOT,    // id
    1,                  // version
    NULL,               // factoryReset
    NULL,               // powerUp
    bootProcessMessage, // processMessage
    NULL,               // poll
    NULL,               // highIsr
    NULL,               // lowIsr
    NULL                // getDiagnostic
};

// Set the EEPROM_BOOT_FLAG to 0 to ensure the application is entered
// after loading using the PicKit.
// Bootflag resides at top of EEPROM.
asm("PSECT eeprom_data,class=EEDATA");
asm("ORG " ___mkstr(EE_TOP));
asm("db 0");

// TODO create a parameter block at 0x820

/**
 * Process the bootloader specific messages. The only message which needs to be 
 * processed is BOOTM (called BOOT).
 * @param m the MERGLCB message
 * @return 1 to indicate that the message has been processed, 0 otherwise
 */
uint8_t bootProcessMessage(Message * m) {
    // check NN matches us
    if (m->bytes[0] != nn.bytes.hi) return 0;
    if (m->bytes[1] != nn.bytes.lo) return 0;
    
    switch (m->opc) {
        case OPC_BOOT:
            // Set the bootloader flag to be picked up by the bootloader
            writeNVM(BOOT_FLAG_NVM_TYPE, BOOT_FLAG_ADDRESS, 0xFF); 
            RESET();     // will enter the bootloader
            return 1;
        default:
            return 0;   // message not processed
    }
}

        