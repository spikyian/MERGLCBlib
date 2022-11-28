#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "mns.h"

#include "ticktime.h"
#include "romops.h"
#include "devincs.h"
#include "timedResponse.h"

#define MNS_VERSION 1

const Service mnsService = {
    SERVICE_ID_MNS,         // id
    1,                      // version
    mnsFactoryReset,        // factoryReset
    mnsPowerUp,             // powerUp
    mnsProcessMessage,      // processMessage
    mnsPoll,                // poll
    NULL,                   // highIsr
    mnsLowIsr,              // lowIsr
    mnsGetDiagnostic        // getDiagnostic
};

// General MNS variables
Word nn;            // node number
uint8_t mode; // operational mode
char * name = NAME; // module name
uint8_t setupModePreviousMode;
Word previousNN;

// LED handling
static LedState    ledState[NUM_LEDS];     // the requested state
static uint8_t flashCounter;     // update every 10ms
static uint8_t flickerCounter;   // update every 10ms
static TickValue ledTimer;
// pb handling
static uint8_t pbState;
static TickValue pbTimer;
static DiagnosticVal mnsDiagnostics[NUM_DIAGNOSTICS];

TimedResponseResult mnsTRserviceDiscoveryCallback(uint8_t type, const Service * s, uint8_t step);
TimedResponseResult mnsTRallDiagnosticsCallback(uint8_t type, const Service * s, uint8_t step);

void mnsFactoryReset(void) {
    uint8_t i;
    nn.bytes.hi = 0;
    nn.bytes.lo = 0;
    writeNVM(NN_NVM_TYPE, NN_ADDRESS, 0);
    writeNVM(NN_NVM_TYPE, NN_ADDRESS+1, 0);
    
    mode = MODE_UNINITIALISED;
    mode = writeNVM(MODE_NVM_TYPE, MODE_ADDRESS, mode);
}

void mnsPowerUp(void) {
    int temp;
    uint8_t i;
    
    temp = readNVM(NN_NVM_TYPE, NN_ADDRESS);
    if (temp < 0) {
        nn.bytes.hi = NN_HI_DEFAULT;
        nn.bytes.lo = NN_LO_DEFAULT;
    } else {
        nn.bytes.hi = (uint8_t)temp;
        temp = readNVM(NN_NVM_TYPE, NN_ADDRESS+1);
        if (temp < 0) {
            nn.bytes.hi = NN_HI_DEFAULT;
            nn.bytes.lo = NN_LO_DEFAULT;
        } else {
            nn.bytes.lo = (uint8_t)temp;
        }
    }
    temp = readNVM(MODE_NVM_TYPE, MODE_ADDRESS);
    if (temp < 0) {
        mode = MODE_DEFAULT;
    } else {
        mode = (uint8_t)temp;
    }
    APP_setPortDirections();
    // Show uninitialised on LEDs
    flashCounter = 0;
    flickerCounter = 0;
    ledTimer.val = 0;
#if NUM_LEDS == 2
    ledState[GREEN_LED] = ON;
    ledState[YELLOW_LED] = OFF;
#endif
#if NUM_LEDS == 1
    ledState[0] = FLASH_50_HALF_HZ;
#endif
    pbState = 0;
    // Clear the diagnostics
    for (i=0; i< NUM_DIAGNOSTICS; i++) {
        mnsDiagnostics[i].asInt = 0;
    } 
}

uint8_t mnsProcessMessage(Message * m) {
    uint8_t i;
    uint8_t flags;
    const Service * s;
    uint8_t newMode;

    // Now do the MNS opcodes
    
    // SETUP mode messages
    if (mode == MODE_SETUP) {
        switch (m->opc) {
            case OPC_SNN:   // Set node number
                nn.bytes.hi = m->bytes[0];
                nn.bytes.lo = m->bytes[1];
                sendMessage2(OPC_NNACK, nn.bytes.hi, nn.bytes.lo);
                mode = MODE_NORMAL;
#if NUM_LEDS == 2
                ledState[GREEN_LED] = OFF;
                ledState[YELLOW_LED] = ON;
#endif
#if NUM_LEDS == 1
                ledState[0] = ON;
#endif
                // Update the LEDs
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
    // No NN but in Normal mode or equivalent message processing
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
            sendMessage5(OPC_PNN, nn.bytes.hi,nn.bytes.lo, MANU_MERG, MTYP_MERGLCB, flags);
            return 1;
        case OPC_GSTOP: // General stop
            APP_GSTOP();
            return 1;
    }
    
    // With NN - check it is us
    if (m->bytes[0] != nn.bytes.hi) return 0;
    if (m->bytes[1] != nn.bytes.lo) return 0;
    // NN message processing
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
                    i=CPU;
                    break;
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
#ifdef _PIC18
                    i=0;
#else
                    i = (*(const uint8_t*)0x3FFFFC; // Device revision byte 0
#endif
                    break;
                case PAR_CPUMID+1:
#ifdef _PIC18
                    i=0;
#else
                    i = (*(const uint8_t*)0x3FFFFD; // Device recision byte 1
#endif
                    break;
                case PAR_CPUMID+2:
                    i = *(const uint8_t*)0x3FFFFE;  // Device ID byte 0
                    break;
                case PAR_CPUMID+3:
                    i = *(const uint8_t*)0x3FFFFF;  // Device ID byte 1
                    break;
                case PAR_CPUMAN:	// CPU manufacturer code
                    i=CPUM_MICROCHIP;
                    break;
                case PAR_BETA:		// Beta revision (numeric), or 0 if release
                    i=PARAM_BUILD_VERSION;
                    break;
                default:
                    i=0;
            }
            sendMessage4(OPC_PARAN, nn.bytes.hi, nn.bytes.lo, m->bytes[2], i);
            return 1;
        case OPC_NNRSM: // reset to manufacturer defaults
            factoryReset();
            return 1;
        case OPC_RDGN:  // diagnostics
            if (m->bytes[2] == 0) {
                // a DGN response for all of the services
                startTimedResponse(TIMED_RESPONSE_RDGN, SERVICE_ID_ALL, &mnsTRallDiagnosticsCallback);
            } else {
                s = findService(m->bytes[2]);
                // an ESD for the particular service
                // TODO What additional data for ESD?
                if (m->bytes[3] == 0) {
                    // a DGN for all diagnostics for a particular service
                    startTimedResponse(TIMED_RESPONSE_RDGN, s->serviceNo, &mnsTRallDiagnosticsCallback);
                }
                if (s->getDiagnostic == NULL) {
                    // the service doesn't support diagnostics
                    sendMessage3(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, GRSP_INVALID_DIAGNOSTIC);
                } else {
                    DiagnosticVal * d = s->getDiagnostic(m->bytes[3]);
                    if (d == NULL) {
                        // the requested diagnostic doesn't exist
                        sendMessage3(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, GRSP_INVALID_DIAGNOSTIC);
                    } else {
                        // it was a request for a single diagnost from a single service
                        sendMessage6(OPC_DGN, nn.bytes.hi, nn.bytes.lo, s->serviceNo, m->bytes[3],d->asBytes.hi, d->asBytes.lo);
                    }
                }
            }
            return 1;
        case OPC_RQSD:  // service discovery
            if (m->bytes[2] == 0) {
                // a SD response for all of the services
                startTimedResponse(TIMED_RESPONSE_RQSD, SERVICE_ID_MNS, &mnsTRserviceDiscoveryCallback);
            } else {
                s = findService(m->bytes[2]);
                // an ESD for the particular service
                // TODO What additional data for ESD?
                sendMessage7(OPC_ESD, nn.bytes.hi, nn.bytes.lo, s->serviceNo, 0,0,0,0);
            }
            return 1;
        case OPC_MODE:  // set operating mode
            newMode = m->bytes[2];
            // check current mode
            switch (mode) {
                case MODE_UNINITIALISED:
                    if (newMode == MODE_SETUP) {
                        mode = MODE_SETUP;
                        setupModePreviousMode = MODE_UNINITIALISED;
                        pbTimer.val = tickGet();
                        //start the request for NN
                        sendMessage2(OPC_RQNN, nn.bytes.hi, nn.bytes.lo);
                        // Update the LEDs
#if NUM_LEDS == 2
                        ledState[GREEN_LED] = OFF;
                        ledState[YELLOW_LED] = FLASH_50_1HZ;
#endif
#if NUM_LEDS == 1
                        ledState[0] = FLASH_50_1HZ;
#endif
                    }
                    break;
                case MODE_SETUP:
                    break;
                default:    // NORMAL modes
                    if (newMode == MODE_SETUP) {
                        // Do State transition from Normal to Setup
                        // release the NN
                        sendMessage2(OPC_NNREL, nn.bytes.hi, nn.bytes.lo);
                        previousNN.word = nn.word;  // save the old NN
                        nn.bytes.lo = nn.bytes.hi = 0;
                        //return to setup
                        mode = MODE_SETUP;
                        setupModePreviousMode = MODE_NORMAL;
                        pbTimer.val = tickGet();
                        // Update the LEDs
#if NUM_LEDS == 2
                        ledState[GREEN_LED] = OFF;
                        ledState[YELLOW_LED] = FLASH_50_1HZ;
#endif
#if NUM_LEDS == 1
                        ledState[0] = FLASH_50_1HZ;
#endif
                    } else if (newMode != MODE_UNINITIALISED) {
                        // change between other modes
                        // No special handling - JFDI
                        mode = newMode;
                    }
                    break;
            }
            return 1;
        case OPC_SQU:   // squelch
            // TODO     Handle Squelch
            return 1;
        case OPC_NNRST: // reset CPU
            RESET();
            return 1;
    }
    return 0;
}

void mnsPoll(void) {
    // update the LEDs
    if (tickTimeSince(ledTimer) > TEN_MILI_SECOND) {
        flashCounter++;
        flickerCounter++;
        ledTimer.val = tickGet();
        
    }
#if NUM_LEDS == 2
    switch (ledState[GREEN_LED]) {
        case ON:
            APP_writeLED2(1);
            break;
        case OFF:
            APP_writeLED2(0);
            break;
        case FLASH_50_1HZ:
            // 1Hz (500ms per cycle is a count of 50 
            APP_writeLED2(flashCounter/50); 
            if (flashCounter >= 100) {
                flashCounter = 0;
            }
            break;
        case FLASH_50_HALF_HZ:
            APP_writeLED2(flashCounter/100);
            if (flashCounter >= 200) {
                flashCounter = 0;
            }
            break;
        case SINGLE_FLICKER_ON:
            APP_writeLED2(1);
            if (flickerCounter >= 20) {     // 100ms
                flickerCounter = 0;
                ledState[GREEN_LED] = OFF;
            }
            break;
        case SINGLE_FLICKER_OFF:
            APP_writeLED2(0);
            if (flickerCounter >= 20) {     // 100ms
                flickerCounter = 0;
                ledState[GREEN_LED] = ON;
            }
            break;
    }
#endif
#if ((NUM_LEDS == 1) || (NUM_LEDS == 2))
    switch (ledState[YELLOW_LED]) {
        case ON:
            APP_writeLED1(1);
            break;
        case OFF:
            APP_writeLED1(0);
            break;
        case FLASH_50_1HZ:
            // 1Hz (500ms per cycle is a count of 50 
            APP_writeLED1(flashCounter/50); 
            if (flashCounter >= 100) {
                flashCounter = 0;
            }
            break;
        case FLASH_50_HALF_HZ:
            APP_writeLED1(flashCounter/100);
            if (flashCounter >= 200) {
                flashCounter = 0;
            }
            break;
        case SINGLE_FLICKER_ON:
            APP_writeLED1(1);
            if (flickerCounter >= 20) {     // 100ms
                flickerCounter = 0;
                ledState[YELLOW_LED] = OFF;
            }
            break;
        case SINGLE_FLICKER_OFF:
            APP_writeLED1(0);
            if (flickerCounter >= 20) {     // 100ms
                flickerCounter = 0;
                ledState[YELLOW_LED] = ON;
            }
            break;
    }
#endif
    // Do the mode changes by push button
    switch(mode) {
        case MODE_UNINITIALISED:
            // check the PB status
            if (APP_pbState() == 0) {
                // pb has not been pressed
                pbTimer.val = tickGet();
            } else {
                if (tickTimeSince(pbTimer) > 4*ONE_SECOND) {
                    // Do state transition from Uninitialised to Setup
                    mode = MODE_SETUP;
                    setupModePreviousMode = MODE_UNINITIALISED;
                    pbTimer.val = tickGet();
                    //start the request for NN
                    sendMessage2(OPC_RQNN, nn.bytes.hi, nn.bytes.lo);
                    // Update the LEDs
#if NUM_LEDS == 2
                    ledState[GREEN_LED] = OFF;
                    ledState[YELLOW_LED] = FLASH_50_1HZ;
#endif
#if NUM_LEDS == 1
                    ledState[0] = FLASH_50_1HZ;
#endif
                }
            }
            break;
        case MODE_SETUP:
            // check for 30secs in SETUP
            if (tickTimeSince(pbTimer) > 3*TEN_SECOND) {
                // return to previous mode
                mode = setupModePreviousMode;
                // restore the NN
                if (mode == MODE_NORMAL) {
                    nn.word = previousNN.word;
                    sendMessage2(OPC_NNACK, nn.bytes.hi, nn.bytes.lo);
                }
            }
            break;
        default:    // Normal modes
            // check the PB status
            if (APP_pbState() == 0) {
                // pb has not been pressed
                pbTimer.val = tickGet();
            } else {
                if (tickTimeSince(pbTimer) > 8*ONE_SECOND) {
                    // Do State transition from Normal to Setup
                    // release the NN
                    sendMessage2(OPC_NNREL, nn.bytes.hi, nn.bytes.lo);
                    previousNN.word = nn.word;  // save the old NN
                    nn.bytes.lo = nn.bytes.hi = 0;
                    //return to setup
                    mode = MODE_SETUP;
                    setupModePreviousMode = MODE_NORMAL;
                    pbTimer.val = tickGet();
                    // Update the LEDs
#if NUM_LEDS == 2
                    ledState[GREEN_LED] = OFF;
                    ledState[YELLOW_LED] = FLASH_50_1HZ;
#endif
#if NUM_LEDS == 1
                    ledState[0] = FLASH_50_1HZ;
#endif
                }
            } 
    }
}

void mnsLowIsr(void) {
    // Tick Timer interrupt
    //check to see if the symbol timer overflowed
    if(TMR_IF)
    {
        /* there was a timer overflow */
        TMR_IF = 0;
        timerExtension1++;
        if(timerExtension1 == 0) {
            timerExtension2++;
        }
    }
    return;
}
DiagnosticVal * mnsGetDiagnostic(uint8_t index) {
    if ((index<1) || (index>NUM_DIAGNOSTICS)) {
        return NULL;
    }
    return &(mnsDiagnostics[index-1]);
}

TimedResponseResult mnsTRserviceDiscoveryCallback(uint8_t type, const Service * s, uint8_t step) {
    if (step >= NUM_SERVICES) {
        return TIMED_RESPONSE_RESULT_FINISHED;
    }
    if (services[step] != NULL) {
        sendMessage4(OPC_SD, nn.bytes.hi, nn.bytes.lo, services[step]->serviceNo, services[step]->version);
    }
    return TIMED_RESPONSE_RESULT_NEXT;
}

TimedResponseResult mnsTRallDiagnosticsCallback(uint8_t type, const Service * s, uint8_t step) {
    if (s->getDiagnostic == NULL) {
        return TIMED_RESPONSE_RESULT_FINISHED;
    }
    DiagnosticVal * d = s->getDiagnostic(step);
    if (d == NULL) {
        // the requested diagnostic doesn't exist
        return TIMED_RESPONSE_RESULT_FINISHED;
    } else {
        // it was a request for a single diagnostic from a single service
        sendMessage6(OPC_DGN, nn.bytes.hi, nn.bytes.lo, s->serviceNo, step,d->asBytes.hi, d->asBytes.lo);
    }
    return TIMED_RESPONSE_RESULT_NEXT;
}
