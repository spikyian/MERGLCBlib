#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "can.h"
#include "mns.h"

Service canService = {
    SERVICE_ID_CAN,     // id
    1,                  // version
    canFactoryReset,    // factoryReset
    canPowerUp,         // powerUp
    canProcessMessage,  // processMessage
    canPoll,            // poll
    canIsr,             // highIsr
    NULL                // lowIsr
};
uint8_t canId;

// forward declarations
uint8_t messageAvailable(void);

//CAN 

void canFactoryReset(void) {
    canId = 1;
    writeNVM(CANID_NVM_TYPE, CANID_ADDRESS, canId);
}

void canPowerUp(void) {
    int temp;
    // initialise the CAN peripheral
    
    temp = readNVM(CANID_NVM_TYPE, CANID_ADDRESS);
    if (temp < 0) {
        // Unsure what to do
        canId = CANID_DEFAULT;
    } else {
        canId = (uint8_t)temp;
    }
}

#define CAN_NNHI     bytes[0]
#define CAN_NNLO     bytes[1]

uint8_t canProcessMessage(Message * m) {
    // check NN matches us
    if (m->CAN_NNHI != nn.bytes.hi) return 0;
    if (m->CAN_NNLO != nn.bytes.lo) return 0;
    
    // Handle any CAN specific OPCs
    switch (m->opc) {
        case OPC_ENUM:
        case OPC_CANID:
            return 1;
    }
    return 0;
}

void canPoll(void) {
    Message * m;
    if (messageAvailable()) {
        m = getReceiveBuffer();
        if (m == NULL) {
            // No RX buffer available
        } else {
            // copy message into buffer
            processMessage(m);
        }
    }
}

uint8_t messageAvailable(void) {
    // TODO     check to see if CAN frame available
    return 1;
}

void canIsr(void) {
    // If RX then transfer frame from CAN peripheral to RX message buffer
    // handle enumeration frame
    // check for CANID clash and start self enumeration process
    
    // If TX then transfer next frame from TX buffer to CAN peripheral 
    
}