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

  Ian Hogg Dec 2022
 */

/**
 * Handle the production of events.
 * If EVENT_HASH_TABLE is defined then an additional lookup table 
 * uint8_t action2Event[NUM_HAPPENINGS] is used to obtain an Event 
 * using a Happening stored in the first EVs. This table is also populated using 
 * rebuildHashTable(). Given a Happening this table can be used to obtain the 
 * index into the EventTable for the Happening so the Event at that index in the 
 * EventTable can be transmitted.
 */

/*
 * Event producer service
 */
#include <xc.h>
#include "module.h"
#include "merglcb.h"
#include "event_teach.h"
#include "event_producer.h"
#include "mns.h"

// Forward function declarations
static DiagnosticVal * producerGetDiagnostic(uint8_t index);
/*
 * Event Producer service.
 * The service definition object is called eventProducerService.
 * Provides a function to allow the application to send events.
 */
// service definition
const Service eventProducerService = {
    SERVICE_ID_PRODUCER,// id
    1,                  // version
    NULL,               // factoryReset
    NULL,               // powerUp
    NULL,               // processMessage
    NULL,               // poll
    NULL,               // highIsr
    NULL,               // lowIsr
    NULL,               // Get ESD data
    producerGetDiagnostic                // getDiagnostic
};

static DiagnosticVal producerDiagnostics[NUM_PRODUCER_DIAGNOSTICS];

// TODO AREQ stuff
/**
 * Provide the means to return the diagnostic data.
 * @param index the diagnostic index
 * @return a pointer to the diagnostic data or NULL if the data isn't available
 */
static DiagnosticVal * producerGetDiagnostic(uint8_t index) {
    if ((index<1) || (index>NUM_PRODUCER_DIAGNOSTICS)) {
        return NULL;
    }
    return &(producerDiagnostics[index-1]);
}

/**
 * Get the Produced Event to transmit for the specified action.
 * If the same produced action has been provisioned for more than 1 event
 * only the first provisioned event will be returned.
 * 
 * @param happening used to lookup the event to be sent
 * @param onOff TRUE for an ON event, FALSE for an OFF event
 * @return TRUE if the produced event is found
 */
Boolean sendProducedEvent(Happening happening, EventState onOff) {
    Word producedEventNN;
    Word producedEventEN;
    uint8_t opc;
#ifndef EVENT_HASH_TABLE
    uint8_t tableIndex;
#endif

#ifdef EVENT_HASH_TABLE
    if (happening2Event[happening] == NO_INDEX) return FALSE;
    producedEventNN.word = getNN(happening2Event[happening]);
    producedEventEN.word = getEN(happening2Event[happening]);
#else
    for (tableIndex=0; tableIndex < NUM_EVENTS; tableIndex++) {
        if (validStart(tableIndex)) {
            Happening h;
            int16_t ev; 
#if HAPPENING_SIZE == 2
            ev = getEv(tableIndex, 0);
            if (ev < 0) continue;
            h.bytes.hi = (uint8_t) ev;
            ev = getEv(tableIndex, 1);
            if (ev < 0) continue;
            h.bytes.lo = (uint8_t) ev;
            if (h.word == happening.word) {
#endif
#if HAPPENING_SIZE ==1
            ev = getEv(tableIndex, 0);
            if (ev < 0) continue;
            h = (uint8_t) ev;
            if (h == happening) {
#endif
                producedEventNN.word = getNN(tableIndex);
                producedEventEN.word = getEN(tableIndex);
#endif
                if (producedEventNN.word == 0) {
                    // Short event
                    if (onOff == EVENT_ON) {
                        opc = OPC_ASON;
                    } else {
                        opc = OPC_ASOF;
                    }
                    producedEventNN.word = nn.word;
                } else {
                    // Long event
                    if (onOff == EVENT_ON) {
                        opc = OPC_ACON;
                    } else {
                        opc = OPC_ACOF;
                    }
                }
                sendMessage4(opc, producedEventNN.bytes.hi, producedEventNN.bytes.lo, producedEventEN.bytes.hi, producedEventEN.bytes.lo);
                producerDiagnostics[PRODUCER_DIAG_NUMPRODUCED].asUint++;
                return TRUE;
#ifndef EVENT_HASH_TABLE
            }
        }
    }
#endif
    return FALSE;
}

