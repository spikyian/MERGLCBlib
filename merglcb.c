#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "service.h"

#define MNS_VERSION 1

// Tx and Rx buffers
static Message rxBuffers[NUM_RXBUFFERS];
static unsigned char rxBufferReadIndex;
static unsigned char rxBufferWriteIndex;
static Message txBuffers[NUM_TXBUFFERS];
static unsigned char txBufferReadIndex;
static unsigned char txBufferWriteIndex;

Word NN;
unsigned char mode;

void factoryReset(void) {
    unsigned char i;
    NN.hi = 0;
    NN.lo = 0;
    writeNVM(NN_NVM_TYPE, NN_ADDRESS, 0);
    writeNVM(NN_NVM_TYPE, NN_ADDRESS+1, 0);
    
    mode = MODE_SETUP;
    mode = writeNVM(MODE_NVM_TYPE, mode);
    
    for (i=0; i< NUMSERVICES; i++) {
        if (services[i]->factoryReset != NULL) {
            services[i]->factoryReset();
        }
    }
}

void powerUp(void) {
    // initialise the RX buffers
    rxBufferReadIndex = 0;
    rxBufferWriteIndex = 0;
    // initialise the TX buffers
    txBufferReadIndex = 0;
    txBufferWriteIndex = 0;
    
    NN.hi = readNVM(NN_NVM_TYPE, NN_ADDRESS);
    NN.lo = readNVM(NN_NVM_TYPE, NN_ADDRESS+1);
    
    mode = readNVM(MODE_NVM_TYPE, MODE_ADDRESS);
    
    unsigned char i;
    for (i=0; i< NUMSERVICES; i++) {
        if (services[i]->powerUp != NULL) {
            services[i]->powerUp();
        }
    }
}

void processMessage(Message * m) {
    unsigned char i;
    unsigned char flags;
    Service * s;
    
    for (i=0; i< NUMSERVICES; i++) {
        if (services[i]->processMessage != NULL) {
            if (services[i]->processMessage(m)) return;
        }
    }
    // Now do the MNS opcodes
    if (mode == MODE_SETUP) {
        switch (m->opc) {
            case OPC_SNN:   // Set node number
                // TODO
                return;
            case OPC_RQNP:  // request parameters
                // TODO
                return;
            case OPC_RQMN:  // Request name
                // TODO
                return;
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
                sendMessage(OPC_PNN, 0,0, MANU_MERG, MTYP_MERGLCB, flags);
                return;
        }
        return;
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
                flags != 32;    // LEARN BIT
            }
            sendMessage(OPC_PNN, NN.hi,NN.lo, MANU_MERG, MTYP_MERGLCB, flags);
            return;
        case OPC_GSTOP: // General stop
            APP_GSTOP();
            return;
    }
    
    // With NN - check it is us
    if (m->bytes[0] != NN.hi) return 0;
    if (m->bytes[1] != NN.lo) return 0;
    switch (m->opc) {
        case OPC_RQNPN: // request node parameter
            // TODO
            return;
        case OPC_NNRSM: // reset to manufacturer defaults
            // TODO
            return;
        case OPC_RDGN:  // diagnostics
            // TODO
            return;
        case OPC_RQSD:  // service discovery
            if (m->bytes[2] == 0) {
                // a SD response for all of the services
                // TODO need to use timedResponse
                sendMessage(OPC_SD, NN.hi, NN.lo, SERVICE_ID_MNS, MNS_VERSION);
                for (i=0; i<NUMSERVICES; i++) {
                    sendMessage(OPC_SD, NN.hi, NN.lo, services[i]->serviceNo, services[i]->version);
                }
            } else {
                s = findService(m->bytes[2]);
                // an ESD for the particular service
                // TODO What data?
                sendMessage(OPC_ESD, NN.hi, NN.lo, s->serviceNo, 0,0,0,0);
            }
            return;
        case OPC_MODE:  // set operating mode
            // TODO
            return;
        case OPC_SQU:   // squelch
            // TODO
            return;
        case OPC_NNRST: // reset CPU
            RESET();
            return;
    }
}

void sendMessage(unsigned char opc){
    sendMessage(opc, 0, 0,0,0,0,0,0,0);
}

void sendMessage(unsigned char opc, unsigned char data1){
    sendMessage(opc, 1, data1, 0,0,0,0,0,0);
}
void sendMessage(unsigned char opc, unsigned char data1, unsigned char data2){
    sendMessage(opc, 2, data1, data2, 0,0,0,0,0);
}
void sendMessage(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3) {
    sendMessage(opc, 3, data1, data2, data3, 0,0,0,0);
}
void sendMessage(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4){
    sendMessage(opc, 4, data1, data2, data3, data4, 0,0,0);
}
void sendMessage(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5) {
    sendMessage(opc, 5, data1, data2, data3, data4, data5, 0,0);
}
void sendMessage(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6) {
    sendMessage(opc, 6, data1, data2, data3, data4, data5, data6,0);
}
void sendMessage(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6, unsigned char data7) {
    sendMessage(opc, 7, data1, data2, data3, data4, data5, data6, data7);
}

void sendMessage(unsigned char opc, unsigned char len, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6, unsigned char data7) {
    Message * m;
    // write the message into the TX buffers
    if (txBufferWriteIndex) {
        // no tx buffers available
    }
    // disable Interrupts
    di();
    m = &(txBuffers[++txBufferWriteIndex]);
    m->opc = opc;
    m->len = len;
    m->bytes[0] = data1;
    m->bytes[1] = data2;
    m->bytes[2] = data3;
    m->bytes[3] = data4;
    m->bytes[4] = data5;
    m->bytes[5] = data6;
    m->bytes[6] = data7;
    // enable Interrupts
    ei();
}
