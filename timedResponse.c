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
**************************************************************************************************************
	The FLiM routines have no code or definitions that are specific to any
	module, so they can be used to provide FLiM facilities for any module 
	using these libraries.
	
*/ 

#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "timedResponse.h"
#include "romops.h"
/* 
 * File:   timedResponse.c
 * Author: Ian
 *
 * Created on 8 December 2021, 16:19
 */
/**
 * TimedResponse handles the sending of CBUS responses at a slower rate to allow receivers of these messages to
 * process them.
 * 
 * Two global variables are used to control the process:
 * <ul>
 * <li>timedResponse determines which set of responses is currently being sent.
 * <li>timedResonseStep determines how far through the responses we're currently at
 * </ul>
 * 
 * Call initTimedResponse() before other TimedResponse processing. This will set the current response set to be None.
 * Call doTimedResponse() on a regular basis which will do the work needed for the next step and increments the step counter.
 * 
 * To start send a set of timed responses set the type of response required and set the step to 0. 
 * For example to send responses to NERD do:
 *  timedResponse = TIMED_RESPONSE_NERD;
 *  timedResponseStep = 0;
 */
static uint8_t timedResponseServiceIndex;
static uint8_t timedResponseAllServicesFlag;
static uint8_t timedResponseType;
static uint8_t timedResponseStep;
static TimedResponseCallback timedResponseCallback;


/**
 * Initialise the timedResponse functionality.
 */
void initTimedResponse(void) {
    timedResponseType = TIMED_RESPONSE_NONE;
}

/*
 Called by services or app to start a timed response 
 */
void startTimedResponse(uint8_t type, uint8_t serviceId, TimedResponseCallback callback) {
    int16_t si;
    timedResponseType = type;
    if (serviceId == SERVICE_ID_ALL) {
        timedResponseAllServicesFlag = 1;
        timedResponseServiceIndex = 0;
    } else {
        timedResponseAllServicesFlag = 0;
        si = findServiceIndex(serviceId);
        if (si < 0) {
            // if we don't have the requested service then don't do anything
            timedResponseType = TIMED_RESPONSE_NONE;
            return;
        }
        timedResponseServiceIndex = (uint8_t)si;
    }
    timedResponseStep = 0;
    timedResponseCallback = callback;
}

// services must provide a function which returns a function pointer when given a type
void pollTimedResponse() {
    if (timedResponseType == TIMED_RESPONSE_NONE) {
        return;
    }
    if (timedResponseCallback == NULL) {
        // no callback defined so finish
        timedResponseType = TIMED_RESPONSE_NONE;
        return;
    }

    // Now call the callback function
    switch (timedResponseCallback(timedResponseType, services[timedResponseServiceIndex], timedResponseStep)) {
        case TIMED_RESPONSE_RESULT_FINISHED:
            // the callback tells us it has finished but lets check if there are other
            // services still to do
            if (timedResponseAllServicesFlag) {
                // move on to next service
                timedResponseServiceIndex++;
                if (timedResponseServiceIndex >= NUM_SERVICES) {
                    // finished all the services
                    timedResponseType = TIMED_RESPONSE_NONE;
                    timedResponseStep = 0;
                }
            } else {
                timedResponseType = TIMED_RESPONSE_NONE;
                timedResponseStep = 0;
            }
            break;
        case TIMED_RESPONSE_RESULT_RETRY:
            break;
        case TIMED_RESPONSE_RESULT_NEXT:
            timedResponseStep++;
            break;
    }
}






/**
 * Send one response CBUS message and increment the step counter ready for the next call.
 * Although I agreed with Pete that SOD is only applicable to EV#2 I actually allow it at any EV#
 */
/*
void doTimedResponse(void) {

    switch (timedResponse) {
        case TIMED_RESPONSE_SOD:
            switch (APP_doSOD(timedResponseStep)) {
                case TIMED_RESPONSE_APP_FINISHED:
                    // say we have finished
                    timedResponse = TIMED_RESPONSE_NONE;
                    timedResponseStep = 0;
                    return;
                case TIMED_RESPONSE_APP_RETRY:
                    // just return so we don't increment step
                    return;
                // case TIMED_RESPONSE_APP_NEXT:
                // just drop out of break and increment step
            }
            break;
        case TIMED_RESPONSE_NERD:
            // The step is used to index through the event table
            if (timedResponseStep >= NUM_EVENTS) {  // finished?
                timedResponse = TIMED_RESPONSE_NONE;
                timedResponseStep = 0;
                return;
            }
            // if its not free and not a continuation then it is start of an event
            if (validStart(timedResponseStep)) {
                WORD n = getNN(timedResponseStep);
                cbusMsg[d3] = n >> 8;
                cbusMsg[d4] = n & 0xFF;
            
                n = getEN(timedResponseStep);
                cbusMsg[d5] = n >> 8;
                cbusMsg[d6] = n & 0xFF;
            
                cbusMsg[d7] = tableIndexToEvtIdx(timedResponseStep); 
                if (!cbusSendOpcMyNN( 0, OPC_ENRSP, cbusMsg )) {
                    // we were unable to send the message so don't update step so that we can try again next time
                    return;
                }
            }
            break;
    }
    timedResponseStep++;
}
*/
