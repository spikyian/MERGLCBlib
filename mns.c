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

/**
 * This is the Minimum Node Specification Service.
 * This is a large service as it handles mode transitions, including setting of 
 * node number. The module's LEDs and push button are also supported.
 * Service discovery and Diagnostsics are also processed from this service.
 * 
 */
#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "mns.h"

#include "ticktime.h"
#include "romops.h"
#include "devincs.h"
#include "timedResponse.h"

#define MNS_VERSION 1

// Forward declarations
void mnsFactoryReset(void);
void mnsPowerUp(void);
void mnsPoll(void);
Processed mnsProcessMessage(Message * m);
void mnsLowIsr(void);
DiagnosticVal * mnsGetDiagnostic(uint8_t index);

/**
 *  The descriptor for the MNS service.
 */
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
/**
 * Node number.
 */
Word nn;            // node number
/**
 * Module operating mode.
 */
uint8_t mode; // operational mode
/**
 * The module's name. The NAME macro should be specified by the application 
 * code in module.h.
 */
char * name = NAME; // module name
// mode transition variables if timeout occurs
/**
 * The previous mode so it can be restored if a timeout occurs.
 */
static uint8_t setupModePreviousMode;
/**
 * The previous node number so it can be restored if a timeout occurs.
 */
static Word previousNN;

// LED handling
/**
 * NUM_LEDS must be specified by the application in module.h.
 * Each LED has a state here to indicate if it is on/off/flashing etc.
 */
static LedState    ledState[NUM_LEDS];     // the requested state
/**
 * Counters to control on/off period.
 */
static uint8_t flashCounter;     // update every 10ms
static uint8_t flickerCounter;   // update every 10ms
static TickValue ledTimer;
/**
 * Module's push button handling.
 * Other UI options are not currently supported.
 */
static uint8_t pbState;
static TickValue pbTimer;
/**
 * The diagnostic values supported by the MNS service.
 */
static DiagnosticVal mnsDiagnostics[NUM_MNS_DIAGNOSTICS];

/* Heartbeat controls */
static uint8_t heartbeatSequence;
TickValue heartbeatTimer;

/**
 * Forward declaration for the TimedResponse callback function for sending
 * Service Discovery responses.
 * @param type type of TimedResponse
 * @param s the service
 * @param step the TimedResponse step
 * @return indication if all the responses have been sent.
 */
TimedResponseResult mnsTRserviceDiscoveryCallback(uint8_t type, const Service * s, uint8_t step);
/**
 * Forward declaration for the TimedResponse callback function for sending
 * Diagnostic responses.
 * @param type type of TimedResponse
 * @param s the service
 * @param step the TimedResponse step
 * @return indication if all the responses have been sent.
 */
TimedResponseResult mnsTRallDiagnosticsCallback(uint8_t type, const Service * s, uint8_t step);

/*
 * The Service functions
 */
/**
 * Perform the MNS factory reset. Just set the node number and mode to default.
 */
void mnsFactoryReset(void) {
    uint8_t i;
    nn.bytes.hi = 0;
    nn.bytes.lo = 0;
    writeNVM(NN_NVM_TYPE, NN_ADDRESS, 0);
    writeNVM(NN_NVM_TYPE, NN_ADDRESS+1, 0);
    
    mode = MODE_UNINITIALISED;
    mode = writeNVM(MODE_NVM_TYPE, MODE_ADDRESS, mode);
}

/**
 * Perform the MNS power up.
 * Loads the node number and mode from non volatile memory. Initialises the LEDs 
 * clear the Diagnostics values.
 */
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
    for (i=0; i< NUM_MNS_DIAGNOSTICS; i++) {
        mnsDiagnostics[i].asInt = 0;
    }
    heartbeatSequence = 0;
    heartbeatTimer.val = 0;
}


/**
 * Minimum Node Specification service MERGCB message processing.
 * This handles all the opcodes for the MNS service. Also handles the mode
 * state transitions and LED changes.
 * 
 * @param m the MERGLCB message to be processed
 * @return PROCESSED if the message was processed, NOT_PROCESSED otherwise
 */
Processed mnsProcessMessage(Message * m) {
    uint8_t i;
    uint8_t flags;
    const Service * s;
    uint8_t newMode;

    // Now do the MNS opcodes
    // TODO check message->len
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
                return PROCESSED;
            case OPC_RQNP:  // request parameters
                sendMessage7(OPC_PARAMS, PARAM_MANU, PARAM_MINOR_VERSION, 
                        PARAM_MODULE_ID, PARAM_NUM_EVENTS, PARAM_NUM_EV_EVENT, 
                        PARAM_NUM_NV, PARAM_MAJOR_VERSION);
                return PROCESSED;
            case OPC_RQMN:  // Request name
                sendMessage7(OPC_NAME, name[0], name[1], name[2], name[3],  
                        name[4], name[5], name[6]);
                return PROCESSED;
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
                return PROCESSED;
            default:
                break;
        }
        return NOT_PROCESSED;
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
            return PROCESSED;
        default:
            break;
    }
    
    // With NN - check it is us
    if (m->bytes[0] != nn.bytes.hi) return NOT_PROCESSED;
    if (m->bytes[1] != nn.bytes.lo) return NOT_PROCESSED;
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
            return PROCESSED;
        case OPC_NNRSM: // reset to manufacturer defaults
            factoryReset();
            return PROCESSED;
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
            return PROCESSED;
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
            return PROCESSED;
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
                    // TODO NOHEARTB mode
                    break;
            }
            return PROCESSED;
        case OPC_SQU:   // squelch
            // TODO     Handle Squelch
            return PROCESSED;
        case OPC_NNRST: // reset CPU
            RESET();
            return PROCESSED;
        default:
            break;
    }
    return NOT_PROCESSED;
}

/**
 * Called regularly, processing for LED flashing and mode state transition 
 * timeouts.
 */
void mnsPoll(void) {
    // Heartbeat message
    if (mode == MODE_NORMAL) {
        // don't send in NOHEARTB mode - or any others
        if (tickTimeSince(heartbeatTimer) > 5*ONE_SECOND) {
            // TODO output the actual status
            sendMessage5(OPC_HEARTB, nn.bytes.hi,nn.bytes.lo,heartbeatSequence++,0,0);
            heartbeatTimer.val = tickGet();
        }
    }
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

/**
 * The MNS interrupt service routine. Handles the tickTime overflow to update
 * the extension bytes.
 */
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

/**
 * Get the MNS diagnostic values.
 * @param index the index indicating which diagnostic is required.
 * @return the Diagnostic value or NULL if the value does not exist.
 */
DiagnosticVal * mnsGetDiagnostic(uint8_t index) {
    if ((index<1) || (index>NUM_MNS_DIAGNOSTICS)) {
        return NULL;
    }
    return &(mnsDiagnostics[index-1]);
}

/**
 * This is the callback used by the service discovery responses.
 * @param type always set to TIMED_RESPONSE_RQSD
 * @param s indicates the service requesting the responses
 * @param step loops through each service to be discovered
 * @return whether all of the responses have been sent yet.
 */
TimedResponseResult mnsTRserviceDiscoveryCallback(uint8_t type, const Service * s, uint8_t step) {
    if (step >= NUM_SERVICES) {
        return TIMED_RESPONSE_RESULT_FINISHED;
    }
//    if (services[step] != NULL) {
        sendMessage4(OPC_SD, nn.bytes.hi, nn.bytes.lo, services[step]->serviceNo, services[step]->version);
//    }
    return TIMED_RESPONSE_RESULT_NEXT;
}

/**
 * This is the callback used by the diagnostic responses.
 * @param type always set to TIMED_RESPONSE_RDNG
 * @param s indicates the service requesting the responses
 * @param step loops through each of the diagnostics
 * @return whether all of the responses have been sent yet.
 */
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
