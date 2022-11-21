#include <xc.h>
#include "service.h"
#include "module.h"

Service canService = {SERVICE_ID_CAN, 1, factoryResetCan, powerUpCan, processMessageCan, pollCan, isrCan};
unsigned char canId;

//CAN 

void factoryResetCan(void) {
    canId = 1;
    writeNVM(CANID_NVM_TYPE, CANID_ADDRESS, canId);
}

void powerUpCan(void) {

    // initialise the CAN peripheral
    
    
    canId = readNVM(CANID_NVM_TYPE, CANID_ADDRESS);
}

#define CAN_NNHI     bytes[0]
#define CAN_NNLO     bytes[1]

unsigned char processMessageCan(Message * m) {
    // check NN matches us
    if (m->CAN_NNHI != NN.hi) return 0;
    if (m->CAN_NNLO != NN.lo) return 0;
    
    // Handle any CAN specific OPCs
    switch (m->opc) {
        case OPC_ENUM:
        case OPC_CANID:
            return 1;
    }
    return 0;
}

void pollCan(void) {
    
}

void isrCan(void) {
    // If RX then transfer frame from CAN peripheral to RX message buffer
    // handle enumeration frame
    // check for CANID clash and start self enumeration process
    
    // If TX then transfer next frame from TX buffer to CAN peripheral 
    
}