#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "mns.h"

#define MNS_VERSION 1

Service mnsService = {
    SERVICE_ID_MNS,         // id
    1,                      // version
    mnsFactoryReset,        // factoryReset
    mnsPowerUp,             // powerUp
    mnsProcessMessage,      // processMessage
    NULL,                   // poll
    NULL,                   // highIsr
    NULL                    // lowIsr
};

Word nn;            // node number
unsigned char mode; // operational mode
char * name = NAME; // module name

void mnsFactoryReset(void) {
    unsigned char i;
    nn.hi = 0;
    nn.lo = 0;
    writeNVM(NN_NVM_TYPE, NN_ADDRESS, 0);
    writeNVM(NN_NVM_TYPE, NN_ADDRESS+1, 0);
    
    mode = MODE_SETUP;
    mode = writeNVM(MODE_NVM_TYPE, MODE_ADDRESS, mode);
}

void mnsPowerUp(void) {
    int temp;
    temp = readNVM(NN_NVM_TYPE, NN_ADDRESS);
    if (temp < 0) {
        nn.hi = NN_HI_DEFAULT;
        nn.lo = NN_LO_DEFAULT;
    } else {
        nn.hi = (unsigned char)temp;
        temp = readNVM(NN_NVM_TYPE, NN_ADDRESS+1);
        if (temp < 0) {
            nn.hi = NN_HI_DEFAULT;
            nn.lo = NN_LO_DEFAULT;
        } else {
            nn.lo = (unsigned char)temp;
        }
    }
    temp = readNVM(MODE_NVM_TYPE, MODE_ADDRESS);
    if (temp < 0) {
        mode = MODE_DEFAULT;
    } else {
        mode = (unsigned char)temp;
    }
}

unsigned char mnsProcessMessage(Message * m) {
    unsigned char i;
    unsigned char flags;
    Service * s;

    // Now do the MNS opcodes
    if (mode == MODE_SETUP) {
        switch (m->opc) {
            case OPC_SNN:   // Set node number
                nn.hi = m->bytes[0];
                nn.lo = m->bytes[1];
                sendMessage2(OPC_NNACK, nn.hi, nn.lo);
                return 1;
            case OPC_RQNP:  // request parameters
                sendMessage7(OPC_PARAMS, PARAM_MANU, PARAM_MINOR_VERSION, 
                        PARAM_MODULE_ID, PARAM_NUM_EVENTS, PARAM_NUM_EV_EVENT, 
                        PARAM_NUM_NV, PARAM_MAJOR_VERSION);
                return 1;
            case OPC_RQMN:  // Request name
                sendMessage7(OPC_NAME, name[0], name[1], name[2], name[3],  
                        name[4], name[5], name[6]);
                return 1;
            case OPC_QNN:   // Query nodes
                flags = 0;
                if (have(SERVICE_ID_CONSUMER)) {
                    flags |= 1;
                }
                if (have(SERVICE_ID_PRODUCER)) {
                    flags |= 2;
                }
                if (flags == 3) flags |= 8;     // CoE
                if (have(SERVICE_ID_BOOT)) {
                    flags |= 16;
                }
                sendMessage5(OPC_PNN, 0,0, MANU_MERG, MTYP_MERGLCB, flags);
                return 1;
        }
        return 0;
    }
    // No NN but in Normal mode or equivalent
    switch (m->opc) {
        case OPC_QNN:   // Query node
            flags = 0;
            if (have(SERVICE_ID_CONSUMER)) {
                flags |= 1; // CONSUMER BIT
            }
            if (have(SERVICE_ID_PRODUCER)) {
                flags |= 2; // PRODUCER BIT
            }
            if (flags == 3) flags |= 8;     // CoE BIT
            flags |= 4; // NORMAL BIT
            if (have(SERVICE_ID_BOOT)) {
                flags |= 16;    // BOOTABLE BIT
            }
            if (mode == MODE_LEARN) {
                flags |= 32;    // LEARN BIT
            }
            sendMessage5(OPC_PNN, nn.hi,nn.lo, MANU_MERG, MTYP_MERGLCB, flags);
            return 1;
        case OPC_GSTOP: // General stop
            APP_GSTOP();
            return 1;
    }
    
    // With NN - check it is us
    if (m->bytes[0] != nn.hi) return 0;
    if (m->bytes[1] != nn.lo) return 0;
    switch (m->opc) {
        case OPC_RQNPN: // request node parameter
            switch(m->bytes[2]) {
                case PAR_NUM:       // Number of parameters
                    i=20;
                    break;
                case PAR_MANU:      // Manufacturer id
                    i=PARAM_MANU;
                    break;
                case PAR_MINVER:	// Minor version letter
                    i=PARAM_MINOR_VERSION;
                    break;
                case PAR_MTYP:	 	// Module type code
                    i=PARAM_MODULE_ID;
                    break;
                case PAR_EVTNUM:	// Number of events supported
                    i=PARAM_NUM_EVENTS;
                    break;
                case PAR_EVNUM:		// Event variables per event
                    i=PARAM_NUM_EV_EVENT;
                    break;
                case PAR_NVNUM:		// Number of Node variables
                    i=PARAM_NUM_NV;
                    break;
                case PAR_MAJVER:	// Major version number
                    i=PARAM_MAJOR_VERSION;
                    break;
                case PAR_FLAGS:		// Node flags
                    i = 0;
                    if (have(SERVICE_ID_CONSUMER)) {
                        i |= 1;
                    }
                    if (have(SERVICE_ID_PRODUCER)) {
                        i |= 2;
                    }
                    if (i == 3) i |= 8;     // CoE
                    flags |= 4; // NORMAL BIT
                    if (have(SERVICE_ID_BOOT)) {
                        i |= 16;
                    }
                    if (mode == MODE_LEARN) {
                        i |= 32;
                    }
                    break;
                case PAR_CPUID:		// Processor type
                case PAR_BUSTYPE:	// Bus type
                    i=0;
                    if (have(SERVICE_ID_CAN)) {
                        i=PB_CAN;
                    }
                    break;
                case PAR_LOAD1:		// load address, 4 bytes
                    i=0x00;
                    break;
                case PAR_LOAD2:		// load address, 4 bytes
                    i=0x08;
                    break;
                case PAR_LOAD3:		// load address, 4 bytes
                    i=0x00;
                    break;
                case PAR_LOAD4:		// load address, 4 bytes
                    i=0x00;
                    break;
                case PAR_CPUMID:	// CPU manufacturer's id as read from the chip config space, 4 bytes (note - read from cpu at runtime, so not included in checksum)
                    // TODO
                case PAR_CPUMAN:	// CPU manufacturer code
                    // TODO
                case PAR_BETA:		// Beta revision (numeric), or 0 if release
                    i=PARAM_BUILD_VERSION;
                    break;
                default:
                    i=0;
            }
            sendMessage4(OPC_PARAN, nn.hi, nn.lo, m->bytes[2], i);
            return 1;
        case OPC_NNRSM: // reset to manufacturer defaults
            factoryReset();
            return 1;
        case OPC_RDGN:  // diagnostics
            // TODO
            return 1;
        case OPC_RQSD:  // service discovery
            if (m->bytes[2] == 0) {
                // a SD response for all of the services
                // TODO need to use timedResponse
                for (i=0; i<NUM_SERVICES; i++) {
                    if (services[i] != NULL) {
                        sendMessage4(OPC_SD, nn.hi, nn.lo, services[i]->serviceNo, services[i]->version);
                    }
                }
            } else {
                s = findService(m->bytes[2]);
                // an ESD for the particular service
                // TODO What data?
                sendMessage7(OPC_ESD, nn.hi, nn.lo, s->serviceNo, 0,0,0,0);
            }
            return 1;
        case OPC_MODE:  // set operating mode
            // TODO
            return 1;
        case OPC_SQU:   // squelch
            // TODO
            return 1;
        case OPC_NNRST: // reset CPU
            RESET();
            return 1;
    }
    return 0;
}