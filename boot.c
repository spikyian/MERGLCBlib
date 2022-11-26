#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "boot.h"
#include "mns.h"

// service definition
Service bootService = {
    SERVICE_ID_BOOT,    // id
    1,                  // version
    NULL,               // factoryReset
    NULL,               // powerUp
    bootProcessMessage, // processMessage
    NULL,               // poll
    NULL,               // highIsr
    NULL                // lowIsr
};


uint8_t bootProcessMessage(Message * m) {
    // check NN matches us
    if (m->bytes[0] != nn.bytes.hi) return 0;
    if (m->bytes[1] != nn.bytes.lo) return 0;
    
    switch (m->opc) {
        case OPC_BOOT:
            writeNVM(EEPROM_NVM_TYPE, BOOT_FLAG_ADDRESS, 0xFF); // Set the bootloader flag to be picked up by the bootloader
            RESET();        // will enter the bootloader
            return 1;
        default:
            return 0;   // message not processed
    }
}

        