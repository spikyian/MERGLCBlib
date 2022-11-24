#include <xc.h>
#include "merglcb.h"
#include "module.h"

// Tx and Rx buffers
static Message rxBuffers[NUM_RXBUFFERS];
static unsigned char rxBufferReadIndex;
static unsigned char rxBufferWriteIndex;
static Message txBuffers[NUM_TXBUFFERS];
static unsigned char txBufferReadIndex;
static unsigned char txBufferWriteIndex;

Service * services[NUM_SERVICES];

/////////////////////////////////////////////
// SERVICE CHECKING FUNCTIONC
/////////////////////////////////////////////
Service * findService(unsigned char id) {
    unsigned char i;
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->serviceNo == id)) {
            return services[i];
        }
    }
    return NULL;
}

unsigned char have(unsigned char id) {
    unsigned char i;
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->serviceNo == id)) {
            return 1;
        }
    }
    return 0;
}

/////////////////////////////////////////////
//FUNCTIONS TO CALL SERVICES
/////////////////////////////////////////////
void factoryReset(void) {
    unsigned char i;
 
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->factoryReset != NULL)) {
            services[i]->factoryReset();
        }
    }
    // now write the version number
    writeNVM(MODE_NVM_TYPE, NV_ADDRESS, APP_NVM_VERSION);
}

void powerUp(void) {
    // initialise the RX buffers
    rxBufferReadIndex = 0;
    rxBufferWriteIndex = 0;
    // initialise the TX buffers
    txBufferReadIndex = 0;
    txBufferWriteIndex = 0;
    
    unsigned char i;
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->powerUp != NULL)) {
            services[i]->powerUp();
        }
    }
}

void processMessage(Message * m) {
    unsigned char i;
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->processMessage != NULL)) {
            if (services[i]->processMessage(m)) return;
        }
    }
}

void poll(void) {
    unsigned char i;
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->poll != NULL)) {
            services[i]->poll();
        }
    }
}

void highIsr(void) {
    unsigned char i;
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->highIsr != NULL)) {
            services[i]->highIsr();
        }
    }
}
void lowIsr(void) {
    unsigned char i;
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->lowIsr != NULL)) {
            services[i]->lowIsr();
        }
    }
}

/////////////////////////////////////////////
// Message handling functions
/////////////////////////////////////////////
void sendMessage0(unsigned char opc){
    sendMessage(opc, 0, 0,0,0,0,0,0,0);
}

void sendMessage1(unsigned char opc, unsigned char data1){
    sendMessage(opc, 1, data1, 0,0,0,0,0,0);
}
void sendMessage2(unsigned char opc, unsigned char data1, unsigned char data2){
    sendMessage(opc, 2, data1, data2, 0,0,0,0,0);
}
void sendMessage3(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3) {
    sendMessage(opc, 3, data1, data2, data3, 0,0,0,0);
}
void sendMessage4(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4){
    sendMessage(opc, 4, data1, data2, data3, data4, 0,0,0);
}
void sendMessage5(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5) {
    sendMessage(opc, 5, data1, data2, data3, data4, data5, 0,0);
}
void sendMessage6(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6) {
    sendMessage(opc, 6, data1, data2, data3, data4, data5, data6,0);
}
void sendMessage7(unsigned char opc, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6, unsigned char data7) {
    sendMessage(opc, 7, data1, data2, data3, data4, data5, data6, data7);
}

void sendMessage(unsigned char opc, unsigned char len, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6, unsigned char data7) {
    Message * m;
    // write the message into the TX buffers TODO check this
    if (txBufferWriteIndex < txBufferReadIndex) {
        // TODO no tx buffers available
    }
    // disable Interrupts
    di();
    txBufferWriteIndex++;
    if (txBufferWriteIndex >= NUM_TXBUFFERS) {
        txBufferWriteIndex = 0;
    }
    m = &(txBuffers[txBufferWriteIndex]);
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

/////////////////////////////////////////////
//BUFFER MANIPULATION FUNCTIONS
/////////////////////////////////////////////
Message * getReceiveBuffer() {
    Message * m;
    m = &(rxBuffers[rxBufferWriteIndex++]);
    if (rxBufferWriteIndex >= NUM_RXBUFFERS) {
        rxBufferWriteIndex=0;
    }
    return m;
}

/////////////////////////////////////////////
// NON VOLATILE MEMORY FUNCTIONS
/////////////////////////////////////////////
unsigned char writeNVM(unsigned char type, unsigned int index, unsigned char value) {
    switch(type) {
        case EEPROM_NVM_TYPE:
            // TODO
        case FLASH_NVM_TYPE:
            // TODO
            break;
    }
}
int readNVM(unsigned char type, unsigned int index) {
    switch(type) {
        case EEPROM_NVM_TYPE:
            // TODO
        case FLASH_NVM_TYPE:
            // TODO
            return 0;
    }
}
