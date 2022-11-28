#include <xc.h>
#include "merglcb.h"
#include "module.h"

#include "romops.h"
#include "ticktime.h"
#include "timedResponse.h"

// Timer 0 is used for Tick time

// Tx and Rx buffers
static Message rxBuffers[NUM_RXBUFFERS];
static uint8_t rxBufferReadIndex;
static uint8_t rxBufferWriteIndex;
static Message txBuffers[NUM_TXBUFFERS];
static uint8_t txBufferReadIndex;
static uint8_t txBufferWriteIndex;

const Service * services[NUM_SERVICES];   // Services stored in Flash
static TickValue timedResponseTime;

/////////////////////////////////////////////
// SERVICE CHECKING FUNCTIONC
/////////////////////////////////////////////
const Service * findService(uint8_t id) {
    uint8_t i;
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->serviceNo == id)) {
            return services[i];
        }
    }
    return NULL;
}

int16_t findServiceIndex(uint8_t id) {
    uint8_t i;
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->serviceNo == id)) {
            return i;
        }
    }
    return -1;
}

uint8_t have(uint8_t id) {
    uint8_t i;
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
    uint8_t i;
 
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->factoryReset != NULL)) {
            services[i]->factoryReset();
        }
    }
    // now write the version number
    writeNVM(MODE_NVM_TYPE, NV_ADDRESS, APP_NVM_VERSION);
}

void powerUp(void) {
    uint8_t i;
    uint8_t divider;
        
    // initialise the RX buffers
    rxBufferReadIndex = 0;
    rxBufferWriteIndex = 0;
    // initialise the TX buffers
    txBufferReadIndex = 0;
    txBufferWriteIndex = 0;
    
    // Initialise the Tick timer. Uses low priority interrupts
    initTicker(0);
    initTimedResponse();
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->powerUp != NULL)) {
            services[i]->powerUp();
        }
    }
}

void processMessage(Message * m) {
    uint8_t i;
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->processMessage != NULL)) {
            if (services[i]->processMessage(m)) return;
        }
    }
    // TODO call the application's processMessage
}

void poll(void) {
    uint8_t i;
    
    /* handle any timed responses */
    if (tickTimeSince(timedResponseTime) > 5*FIVE_MILI_SECOND) {
        pollTimedResponse();
        timedResponseTime.val = tickGet();
    }
    /* call any service polls */
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->poll != NULL)) {
            services[i]->poll();
        }
    }
}

void highIsr(void) {
    uint8_t i;
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->highIsr != NULL)) {
            services[i]->highIsr();
        }
    }
}
void lowIsr(void) {
    uint8_t i;
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->lowIsr != NULL)) {
            services[i]->lowIsr();
        }
    }
}

/////////////////////////////////////////////
// Message handling functions
/////////////////////////////////////////////
void sendMessage0(uint8_t opc){
    sendMessage(opc, 0, 0,0,0,0,0,0,0);
}

void sendMessage1(uint8_t opc, uint8_t data1){
    sendMessage(opc, 1, data1, 0,0,0,0,0,0);
}
void sendMessage2(uint8_t opc, uint8_t data1, uint8_t data2){
    sendMessage(opc, 2, data1, data2, 0,0,0,0,0);
}
void sendMessage3(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3) {
    sendMessage(opc, 3, data1, data2, data3, 0,0,0,0);
}
void sendMessage4(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4){
    sendMessage(opc, 4, data1, data2, data3, data4, 0,0,0);
}
void sendMessage5(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5) {
    sendMessage(opc, 5, data1, data2, data3, data4, data5, 0,0);
}
void sendMessage6(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6) {
    sendMessage(opc, 6, data1, data2, data3, data4, data5, data6,0);
}
void sendMessage7(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6, uint8_t data7) {
    sendMessage(opc, 7, data1, data2, data3, data4, data5, data6, data7);
}

void sendMessage(uint8_t opc, uint8_t len, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6, uint8_t data7) {
    Message * m;
    // write the message into the TX buffers TODO check this
    if (txBufferWriteIndex < txBufferReadIndex) {
        // TODO     handle occurance of no transmit buffers
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

