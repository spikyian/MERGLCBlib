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

/*
 * Event producer service
 */
#include <xc.h>
#include "module.h"
#include "merglcb.h"
#include "event_teach.h"
#include "event_producer.h"
#include "mns.h"
/*
 * Event Producer service
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
    NULL                // getDiagnostic
};



/**
 * Get the Produced Event to transmit for the specified action.
 * If the same produced action has been provisioned for more than 1 event
 * only the first provisioned event will be returned.
 * 
 * @param action the produced action
 * @return TRUE if the produced event is found
 */ 
void sendProducedEvent(Happening happening, uint8_t onOff) {
    Word producedEventNN;
    Word producedEventEN;
#ifndef HASH_TABLE
    uint8_t tableIndex;
#endif

#ifdef HASH_TABLE
    if (happening2Event[happening-HAPPENING_BASE] == NO_INDEX) return FALSE;
    producedEvent.NN = getNN(happening2Event[happening-HAPPENING_BASE]);
    producedEvent.EN = getEN(happening2Event[happening-HAPPENING_BASE]);
    return TRUE;
#else
    for (tableIndex=0; tableIndex < NUM_EVENTS; tableIndex++) {
        if (validStart(tableIndex)) {
            int16_t ev = getEv(tableIndex, 0);
            if (ev < 0) continue;
            if (ev == happening) {
                producedEventNN.word = getNN(tableIndex);
                producedEventEN.word = getEN(tableIndex);

                if (producedEventNN.word == 0) {
                    // Short event
                    if (onOff) {
                        sendMessage4(OPC_ASON, nn.bytes.hi, nn.bytes.lo, producedEventEN.bytes.hi, producedEventEN.bytes.lo);
                    } else {
                        sendMessage4(OPC_ASOF, nn.bytes.hi, nn.bytes.lo, producedEventEN.bytes.hi, producedEventEN.bytes.lo);
                    }
                } else {
                    // Long event
                    if (onOff) {
                        sendMessage4(OPC_ACON, producedEventNN.bytes.hi, producedEventNN.bytes.lo, producedEventEN.bytes.hi, producedEventEN.bytes.lo);
                    } else {
                        sendMessage4(OPC_ACOF, producedEventNN.bytes.hi, producedEventNN.bytes.lo, producedEventEN.bytes.hi, producedEventEN.bytes.lo);
                    }
                } 
                return;
            }
        }
    }
#endif
}

