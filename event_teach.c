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
#include <xc.h>
#include "merglcb.h"
#include "romops.h"
#include "mns.h"
#include "timedResponse.h"
#include "event_teach.h"

/*
 * Event teaching service
 *
 * The events are stored as a hash table in flash (flash is faster to read than EEPROM)
 * There can be up to 255 events. Since the address in the hash table will be 16 bits, and the
 * address of the whole event table can also be covered by a 16 bit address, there is no
 * advantage in having a separate hashed index table pointing to the actual event table.
 * Therefore the hashing algorithm produces the index into the actual event table, which
 * can be shifted to give the address - each event table entry is 16 bytes. After the event
 * number and hash table overhead, this allows up to 10 EVs per event.
 *
 * This generic FLiM code needs no knowledge of specific EV usage except that EV#1 is 
 * to define the Produced events (if the PRODUCED_EVENTS definition is defined).
 *
 * BEWARE must set NUM_EVENTS to a maximum of 255!
 * If set to 256 then the for (uint8_t i=0; i<NUM_EVENTS; i++) loops will never end
 * as they use an uint8_t instead of int for space/performance reasons.
 *
 * BEWARE Concurrency: The functions which use the EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_ and hash/lookup must not be used
 * whilst there is a chance of the functions which modify the EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_ of RAM based 
 * hash/lookup tables being called. These functions should therefore either be called
 * from the same thread or disable interrupts. 
 *
 * define HASH_TABLE to use event hash tables for fast access - at the expense of some RAM
 *
 * The code in event.c is responsible for storing EVs for each defined event and 
 * also for allowing speedy lookup of EVs given an Event or finding an Event given 
 * a Happening which is stored in EV#1.
 * 
 * Events are stored in the EventTable which consists of rows containing the following 
 * fields:
 * * EventTableFlags flags         1 byte
 * * uint8_t next                     1 byte
 * * Event event                   4 bytes
 * * uint8_t evs[EVENT_TABLE_WIDTH]   EVENT_TABLE_WIDTH bytes
 * 
 * The number of table entries is defined by NUM_EVENTS.
 * 
 * The EVENT_TABLE_WIDTH determines the number of EVs that can be stored within 
 * a single EventTable row. Where events can have more EVs that can be fitted within 
 * a single row then multiple rows are chained together using the 'next' field. 
 * 'next' indicates the index into the EventTable for chained entries for a single 
 * Event.
 * 
 * An example to clarify is the CANMIO which sets EVENT_TABLE_WITH to 10 so that 
 * size of a row is 16bytes. A chain of two rows can store 20 EVs. CANMIO has a 
 * limit of 20 EVs per event (EVperEVT) so that a maximum of 2 entries are chained.
 * 
 * The 'event' field is only used in the first in a chain of entries and contains 
 * the NN/EN of the event.
 * 
 * The EventTableFlags have the following entries:
 * eVsUsed                4 bits
 * continued                       1 bit
 * forceOwnNN                      1 bit
 * freeEntry                       1 bit
 * 
 * The 'continued' flag indicates if there is another table entry chained to this 
 * one. If the flag is set then the 'next' field contains the index of the chained 
 * entry.
 * 
 * The 'forceOwnNN' flag indicates that for produced events the NN in the event 
 * field should be ignored and the module's current NN should be used instead. 
 * This allows default events to be maintained even if the NN of the module is 
 * changed. Therefore this flag should be set for default produced events.
 * 
 * The 'freeEntry' flag indicates that this entry in the EventTable is currently 
 * unused.
 * 
 * The 'eVsUsed' field records how many of the evs contain valid data. 
 * It is only applicable for the last entry in the chain since all EVs less than 
 * this are assumed to contain valid data. Since this field is only 4 bits long 
 * this places a limit on the EVENT_TABLE_WIDTH of 15.
 * 
 * EXAMPLE
 * Let's go through an example of filling in the table. We'll look at the first 
 * 4 entries in the table and let's have EVENT_TABLE_WIDTH=4 but have EVperEVT=8.
 * 
 * At the outset there is an empty table. All rows have the 'freeEntry' bit set:
 * 
 * index   eVsUsed        continued       forceOwnNN      freeEntry       next    Event   evs[]
 * 0       0              0               0               1               0       0:0     0,0,0,0
 * 1       0              0               0               1               0       0:0     0,0,0,0
 * 2       0              0               0               1               0       0:0     0,0,0,0
 * 3       0              0               0               1               0       0:0     0,0,0,0
 *
 * Now if an EV of an event is set (probably using EVLRN CBUS command) then the 
 * table is updated. Let's set EV#1 for event 256:101 to the value 99:
 * 
 * index   eVsUsed        continued       forceOwnNN      freeEntry       next    Event   evs[]
 * 0       1              0               0               0               0       256:101 99,0,0,0
 * 1       0              0               0               1               0       0:0     0,0,0,0
 * 2       0              0               0               1               0       0:0     0,0,0,0
 * 3       0              0               0               1               0       0:0     0,0,0,0
 * 
 * Now let's set EV#2 of event 256:102 to 15:
 * 
 * index   eVsUsed        continued       forceOwnNN      freeEntry       next    Event   evs[]
 * 0       1              0               0               0               0       256:101 99,0,0,0
 * 1       2              0               0               0               0       256:102 0,15,0,0
 * 2       0              0               0               1               0       0:0     0,0,0,0
 * 3       0              0               0               1               0       0:0     0,0,0,0
 * 
 * Now let's set EV#8 of event 256:101 to 29:
 * 
 * index   eVsUsed        continued       forceOwnNN      freeEntry       next    Event   evs[]
 * 0       1              1               0               0               0       256:101 99,0,0,0
 * 1       2              0               0               0               0       256:102 0,15,0,0
 * 2       4              0               0               0               0       0:0     0,0,0,29
 * 3       0              0               0               1               0       0:0     0,0,0,0
 * 
 * To perform the speedy lookup of EVs given an Event a hash table can be used by 
 * defining HASH_TABLE. The hash table is stored in 
 * uint8_t eventChains[HASH_LENGTH][CHAIN_LENGTH];
 * 
 * An event hashing function is provided uint8_t getHash(nn, en) which should give 
 * a reasonable distribution of hash values given the typical events used.
 * 
 * This table is populated from the EventTable upon power up using rebuildHashtable(). 
 * This function must be called before attempting to use the hash table. Each Event 
 * from the EventTable is hashed using getHash(nn,en), trimmed to the HASH_LENGTH 
 * and the index in the EventTable is then stored in the eventChains at the next 
 * available bucket position.
 * 
 * When an Event is received from CBUS and we need to find its index within the 
 * EventTable it is firstly hashed using getHash(nn,en), trimmed to HASH_LENGTH 
 * and this is used as the first index into eventChains[][]. We then step through 
 * the second index of buckets within the chain. Each entry is an index into the 
 * EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_ and the EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_'s event field is checked to see if it matches the
 * received event. It it does match then the index into EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_ has been found 
 * and is returned. The EVs can then be accessed from the ev[] field.
 * 
 * If PRODUCED_EVENTS is defined in addition to HASH_TABLE then an additional 
 * lookup table uint8_t action2Event[NUM_HAPPENINGS] is used to obtain an Event 
 * using a Happening (previously called Actions) stored in EV#1. This table is 
 * also populated using rebuildHashTable(). Given a Happening this table can be 
 * used to obtain the index into the EventTable for the Happening so the Event 
 * at that index in the EventTable can be transmitted onto the CBUS.
 */

// forward definitions
static void teachFactoryReset(void);
static void teachPowerUp(void);
static Processed teachProcessMessage(Message * m);
static DiagnosticVal * teachGetDiagnostic(uint8_t code);
static void clearAllEvents(void);
Processed checkLen(Message * m, uint8_t needed);
uint8_t evtIdxToTableIndex(uint8_t evtIdx);
TimedResponseResult nerdCallback(uint8_t type, const Service * s, uint8_t step);
uint8_t validStart(uint8_t tableIndex);
uint16_t getNN(uint8_t tableIndex);
uint16_t getEN(uint8_t tableIndex);
uint8_t numEv(uint8_t tableIndex);
int getEv(uint8_t tableIndex, uint8_t evNum);
uint8_t tableIndexToEvtIdx(uint8_t tableIndex);
uint8_t findEvent(uint16_t nodeNumber, uint16_t eventNumber);
uint8_t removeTableEntry(uint8_t tableIndex);
uint8_t writeEv(uint8_t tableIndex, uint8_t evNum, uint8_t evVal);
uint8_t removeEvent(uint16_t nodeNumber, uint16_t eventNumber);
void checkRemoveTableEntry(uint8_t tableIndex);
void doNnclr(void);
void doNerd(void);
void doNnevn(void);
void doRqevn(void);
void doNenrd(uint8_t index);
void doReval(uint8_t enNum, uint8_t evNum);
void doEvuln(uint16_t nodeNumber, uint16_t eventNumber);
void doReqev(uint16_t nodeNumber, uint16_t eventNumber, uint8_t evNum);
void doEvlrn(uint16_t nodeNumber, uint16_t eventNumber, uint8_t evNum, uint8_t evVal);

// service definition
const Service eventTeachService = {
    SERVICE_ID_TEACH,      // id
    1,                  // version
    teachFactoryReset,  // factoryReset
    teachPowerUp,       // powerUp
    teachProcessMessage,// processMessage
    NULL,               // poll
    NULL,               // highIsr
    NULL,               // lowIsr
    NULL                // getDiagnostic
};

// Space for the event table
const uint8_t eventTable[NUM_EVENTS * EVENTTABLE_ROW_WIDTH] __at(EVENT_TABLE_ADDRESS) ={0xFF};

//
// SERVICE FUNCTIONS
//

/**
 * 
 */
static void teachFactoryReset(void) {
    initRomOps();
    clearAllEvents();
}

/**
 * 
 */
static void teachPowerUp(void) {
#ifdef HASH_TABLE
    rebuildHashtable();
#endif
}

/**
 * 
 * @param m
 * @return 
 */
static Processed teachProcessMessage(Message* m) {
    switch(m->opc) {
        /* First mode changes. Another check if we are going into Learn or another node is going to Learn */
        case OPC_NNLRN:
            if (checkLen(m, 3) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] == nn.bytes.hi) && (m->bytes[1] == nn.bytes.lo)) {
                mode = MODE_LEARN;
            } else if (mode == MODE_LEARN) {
                mode = MODE_NORMAL;
            }
            return PROCESSED;
        case OPC_MODE:      // 76 MODE - NN, mode
            if (checkLen(m, 4) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] == nn.bytes.hi) && (m->bytes[1] == nn.bytes.lo)) {
                if (m->len < 4) {
                    sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_CMD);
                    return PROCESSED;
                }
                if (m->bytes[2] == MODE_LEARN) {
                    // Do enter Learn mode
                    mode = MODE_LEARN;
                } else if (mode == MODE_LEARN) {
                    // Do exit Learn mode
                    if (m->bytes[2] == MODE_NORMAL) {
                        mode = MODE_NORMAL;
                    }
                }
            } else {
                // Another module going to Learn so we must exit learn
                if (mode == MODE_LEARN) {
                    mode = MODE_NORMAL;
                }
            }
            return NOT_PROCESSED;   // mode probably processed by other services
        /* This block must be in Learn mode and NN doesn't need to match ours */
        case OPC_EVLRN:     // D2 EVLRN - NN, EN, EV#, EVval
            if (checkLen(m, 7) == PROCESSED) return PROCESSED;
            if (mode != MODE_LEARN) return PROCESSED;
            // Do learn
            doEvlrn((uint16_t)(m->bytes[0]<<8) | (m->bytes[1]), (uint16_t)(m->bytes[2]<<8) | (m->bytes[3]), m->bytes[4], m->bytes[5]);
            return PROCESSED;
        case OPC_EVULN:     // 95 EVULN - NN, EN
            if (checkLen(m, 5) == PROCESSED) return PROCESSED;
            if (mode != MODE_LEARN) return PROCESSED;
            // do unlearn
            // TODO
            doEvuln((uint16_t)(m->bytes[0]<<8) | (m->bytes[1]), (uint16_t)(m->bytes[2]<<8) | (m->bytes[3]));
            return PROCESSED;
        case OPC_REQEV:     // B2 REQEV - NN EN EV#
            if (checkLen(m, 6) == PROCESSED) return PROCESSED;
            if (mode != MODE_LEARN) return PROCESSED;
            // do read EV
            // TODO
            doReqev((uint16_t)(m->bytes[0]<<8) | (m->bytes[1]), (uint16_t)(m->bytes[2]<<8) | (m->bytes[3]), m->bytes[4]);
            return PROCESSED;
        /* This block contain an NN which needs to match our NN */
        case OPC_NNULN:     // 54 NNULN - NN
            if (checkLen(m, 3) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] != nn.bytes.hi) || (m->bytes[1] != nn.bytes.lo)) return PROCESSED;  // not us
            // Do exit Learn mode
            if (mode == MODE_LEARN) {
                mode = MODE_NORMAL;
            }
            return PROCESSED;
        case OPC_NNCLR:     // 55 NNCLR - NN
            if (checkLen(m, 3) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] != nn.bytes.hi) || (m->bytes[1] != nn.bytes.lo)) return PROCESSED;  // not us
            /* Must be in Learn mode for this one */
            if (mode != MODE_LEARN) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_NOT_LRN);
                return PROCESSED;
            }
            // do NNCLR
            doNnclr();
            break;
        case OPC_NERD:      // 57 NERD - NN
            if (checkLen(m, 3) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] != nn.bytes.hi) || (m->bytes[1] != nn.bytes.lo)) return PROCESSED;  // not us
            // do NERD
            doNerd();
            return PROCESSED;
        case OPC_NNEVN:     // 56 NNEVN - NN
            if (checkLen(m, 3) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] != nn.bytes.hi) || (m->bytes[1] != nn.bytes.lo)) return PROCESSED;  // not us
            // do NNEVN
            doNnevn();
            return PROCESSED;
        case OPC_RQEVN:     // 58 RQEVN - NN
            if (checkLen(m, 3) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] != nn.bytes.hi) || (m->bytes[1] != nn.bytes.lo)) return PROCESSED;  // not us
            // do RQEVN
            doRqevn();
            return PROCESSED;
        case OPC_NENRD:     // 72 NENRD - NN, EN#
            if (checkLen(m, 4) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] != nn.bytes.hi) || (m->bytes[1] != nn.bytes.lo)) return PROCESSED;  // not us
            // do NENRD
            doNenrd(m->bytes[2]);
            return PROCESSED;
        case OPC_REVAL:     // 9C REVAL - NN, EN#, EV#
            if (checkLen(m, 5) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] != nn.bytes.hi) || (m->bytes[1] != nn.bytes.lo)) return PROCESSED;  // not us
            // do REVAL
            // TODO
            doReval(m->bytes[2], m->bytes[3]);
            return PROCESSED;
        case OPC_EVLRNI:    // F5 EVLRNI - NN, EN, EN#, EV#, EVval
            if (checkLen(m, 8) == PROCESSED) return PROCESSED;
            if ((m->bytes[0] != nn.bytes.hi) || (m->bytes[1] != nn.bytes.lo)) return PROCESSED;  // not us
            // do EVLRNI
            // TODO
            doEvlrn((uint16_t)(m->bytes[0]<<8) | (m->bytes[1]), (uint16_t)(m->bytes[2]<<8) | (m->bytes[3]), m->bytes[5], m->bytes[6]);
            return PROCESSED;
        default:
            break;
    }
    return NOT_PROCESSED;
}

/**
 * Checks that the required number of message bytes are present.
 * @param m
 * @param needed
 * @return 
 */
Processed checkLen(Message * m, uint8_t needed) {
    if (m->len < needed) {
        if (m->len > 2) {
            if ((m->bytes[0] == nn.bytes.hi) && (m->bytes[1] == nn.bytes.lo)) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_CMD);
            }
        }
        return PROCESSED;
    }
    return NOT_PROCESSED;
}



//
// FUNCTIONS TO DO THE ACTUAL WORK
//

/**
 * Removes all events including default events.
 */
static void clearAllEvents(void) {
    uint8_t tableIndex;
    for (tableIndex=0; tableIndex<NUM_EVENTS; tableIndex++) {
        // set the free flag
        writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex + EVENTTABLE_OFFSET_FLAGS, 0xff);
    }
    flushFlashBlock();
#ifdef HASH_TABLE
    rebuildHashtable();
#endif
}

/**
 * Read number of available event slots.
 * This returned the number of unused slots in the Consumed event Event2Action table.
 */
void doNnevn(void) {
    // count the number of unused slots.
    uint8_t count = 0;
    uint8_t i;
    for (i=0; i<NUM_EVENTS; i++) {
        EventTableFlags f;
        f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_FLAGS);
        if (f.freeEntry) {
            count++;
        }
    }
    sendMessage3(OPC_EVNLF, nn.bytes.hi, nn.bytes.lo, count);
} // doNnevn


/**
 * Do the NERD. 
 * This sets things up so that timedResponse will do the right stuff.
 */
void doNerd(void) {
    startTimedResponse(OPC_NERD, SERVICE_ID_TEACH, nerdCallback);
}

TimedResponseResult nerdCallback(uint8_t type, const Service * s, uint8_t step){
    Word nodeNumber, eventNumber;
    // The step is used to index through the event table
    if (step >= NUM_EVENTS) {  // finished?
        return TIMED_RESPONSE_RESULT_FINISHED;
    }
    // if its not free and not a continuation then it is start of an event
    if (validStart(step)) {
        nodeNumber.word = getNN(step);
        eventNumber.word = getEN(step);
        sendMessage7(OPC_ENRSP, nn.bytes.hi, nn.bytes.lo, nodeNumber.bytes.hi, nodeNumber.bytes.lo, eventNumber.bytes.hi, eventNumber.bytes.lo, tableIndexToEvtIdx(step));
    }
    return TIMED_RESPONSE_RESULT_NEXT;
}


/**
 * Read a single stored event by index and return a ENRSP response.
 * DEPRECATED
 * 
 * @param index index into event table
 */
void doNenrd(uint8_t index) {
    uint8_t tableIndex;
    uint16_t nodeNumber, eventNumber;
    
    tableIndex = evtIdxToTableIndex(index);
    // check this is a valid index
    if ( ! validStart(tableIndex)) {
        sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INVALID_EVENT);
            // DEBUG  
//        cbusMsg[d7] = readNVM((uint16_t)(& (EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_[tableIndex].flags.asByte))); 
//        cbusSendOpcMyNN( 0, OPC_ENRSP, cbusMsg );
        return;
    }
    nodeNumber = getNN(tableIndex);
    eventNumber = getEN(tableIndex);
    sendMessage5(OPC_ENRSP, nodeNumber>>8, nodeNumber&0xFF, eventNumber>>8, eventNumber&0xFF, index);

} // doNenrd

/**
 * Read number of stored events.
 * This returns the number of events which is different to the number of used slots 
 * in the Event table.
 */
void doRqevn(void) {
    // Count the number of used slots.
    uint8_t count = 0;
    uint8_t i;
    for (i=0; i<NUM_EVENTS; i++) {
        if (validStart(i)) {
            count++;    
        }
    }
    sendMessage3(OPC_NUMEV, nn.bytes.hi, nn.bytes.lo, count);
} // doRqevn

/**
 * Clear all Events.
 */
void doNnclr(void) {
    if (mode == MODE_LEARN) {
        clearAllEvents();
        sendMessage2(OPC_WRACK, nn.bytes.hi, nn.bytes.lo);
    } else {
            sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_NOT_LRN);
 	}
} //doNnclr

/**
 * Teach event whilst in learn mode.
 * Teach or reteach an event associated with an action. 

 * @param nodeNumber
 * @param eventNumber
 * @param evNum the EV number
 * @param evVal the EV value
 */
void doEvlrn(uint16_t nodeNumber, uint16_t eventNumber, uint8_t evNum, uint8_t evVal) {
    uint8_t error;
    // evNum starts at 1 - convert to zero based
    if (evNum == 0) {
        sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_EV_IDX);
        return;
    }
    evNum--;    // convert CBUS numbering (starts at 1) to internal numbering)
    error = APP_addEvent(nodeNumber, eventNumber, evNum, evVal);
    if (error) {
        // validation error
        sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, error);
        return;
    }
    sendMessage2(OPC_WRACK, nn.bytes.hi, nn.bytes.lo);
    return;
}

/**
 * Read an event variable by index.
 */
void doReval(uint8_t enNum, uint8_t evNum) {
	// Get event index and event variable number from message
	// Send response with EV value
    uint8_t evIndex;
    uint8_t tableIndex = evtIdxToTableIndex(enNum);
    
    if (evNum > PARAM_NUM_EV_EVENT) {
        sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_EV_IDX);
        return;
    }
    evIndex = evNum-1U;    // Convert from CBUS numbering (starts at 1 for produced action))
    
    // check it is a valid index
    if (tableIndex < NUM_EVENTS) {
        if (validStart(tableIndex)) {
            int evVal;
            if (evNum == 0) {
                evVal = numEv(tableIndex);
            } else {
                evVal = getEv(tableIndex, evIndex);
            }
            if (evVal >= 0) {
                sendMessage5(OPC_NEVAL, nn.bytes.hi, nn.bytes.lo, enNum, evNum, (uint8_t)evVal);
                return;
            }
            // a negative value is the error code
            sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-evVal));
            return;
        }
    }
    sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INVALID_EVENT);
} // doReval

/**
 * Unlearn event.
 * @param nodeNumber
 * @param eventNumber
 */
void doEvuln(uint16_t nodeNumber, uint16_t eventNumber) {
    removeEvent(nodeNumber, eventNumber);
    // Don't send a WRACK
}

/**
 * Read an event variable by event id.
 * @param nodeNumber
 * @param eventNumber
 * @param evNum
 */
void doReqev(uint16_t nodeNumber, uint16_t eventNumber, uint8_t evNum) {
    int16_t evVal;
    // get the event
    uint8_t tableIndex = findEvent(nodeNumber, eventNumber);
    if (tableIndex == NO_INDEX) {
        sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INVALID_EVENT);
        return;
    }
    if (evNum > PARAM_NUM_EV_EVENT) {
        sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_EV_IDX);
        return;
    }
    if (evNum == 0) {
        evVal = numEv(tableIndex);
    } else {
        evNum--;    // Convert from CBUS numbering (starts at 1 for produced action))
        evVal = getEv(tableIndex, evNum);
    }
    if (evVal < 0) {
        // a negative value is the error code
        sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-evVal));
    }

    sendMessage6(OPC_EVANS, nn.bytes.hi, nn.bytes.lo, eventNumber>>8, eventNumber&0xFF, evNum, (uint8_t)evVal);
    return;
}



/**
 * Remove event.
 * 
 * @param nodeNumber
 * @param eventNumber
 * @return error or 0 for success
 */
uint8_t removeEvent(uint16_t nodeNumber, uint16_t eventNumber) {
    // need to delete this action from the Event table. 
    uint8_t tableIndex = findEvent(nodeNumber, eventNumber);
    if (tableIndex == NO_INDEX) return CMDERR_INV_EV_IDX; // not found
    // found the event to delete
    return removeTableEntry(tableIndex);
}

/**
 * 
 * @param tableIndex
 * @return 
 */
uint8_t removeTableEntry(uint8_t tableIndex) {
    EventTableFlags f;

#ifdef SAFETY
    if (tableIndex >= NUM_EVENTS) return CMDERR_INV_EV_IDX;
#endif
    if (validStart(tableIndex)) {
        // set the free flag
        writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS, 0xff);
        // Now follow the next pointer
        f.asByte = 0xff;
        while (f.continued) {
            tableIndex = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NEXT);
            f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
        
            if (tableIndex >= NUM_EVENTS) return CMDERR_INV_EV_IDX; // shouldn't be necessary
        
            // the continuation flag of this entry should be set but I'm 
            // not going to check as I wouldn't know what to do if it wasn't set
                    
            // set the free flag
            writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS, 0xff);
        
        }
        flushFlashBlock();
#ifdef HASH_TABLE
        // easier to rebuild from scratch
        rebuildHashtable();
#endif
    }
    return 0;
}

/**
 * Add an event/EV.
 * Teach or re-teach an EV for an event. 
 * This may (optionally) need to create a new event and then optionally
 * create additional chained entries. All newly allocated table entries need
 * to be initialised.
 * 
 * @param nodeNumber
 * @param eventNumber
 * @param evNum the EV index (starts at 0 for the produced action)
 * @param evVal the EV value
 * @param forceOwnNN the value of the flag
 * @return error number or 0 for success
 */
uint8_t addEvent(Word nodeNumber, Word eventNumber, uint8_t evNum, uint8_t evVal, uint8_t forceOwnNN) {
    uint8_t tableIndex;
    uint8_t error;
    // do we currently have an event
    tableIndex = findEvent(nodeNumber.word, eventNumber.word);
    if (tableIndex == NO_INDEX) {
        // Ian - 2k check for special case. Don't create an entry for a NO_ACTION
        // This is a solution to the problem of FCU filling the event table with unused
        // 00 Actions. 
        // It does not fix a much less frequent problem of releasing some of the 
        // table entries when they are filled with No Action.
        if (evVal == EV_FILL) {
            return 0;
        }
        error = 1;
        // didn't find the event so find an empty slot and create one
        for (tableIndex=0; tableIndex<NUM_EVENTS; tableIndex++) {
            EventTableFlags f;
            f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
            if (f.freeEntry) {
                uint8_t e;
                // found a free slot, initialise it
                writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NN, nodeNumber.bytes.hi);
                writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NN+1, nodeNumber.bytes.lo);
                writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_EN, eventNumber.bytes.hi);
                writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_EN+1, eventNumber.bytes.lo);
                f.asByte = 0;
                f.forceOwnNN = forceOwnNN;
                writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS, f.asByte);
            
                for (e = 0; e < EVENT_TABLE_WIDTH; e++) {
                    writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_EVS+e, EV_FILL);
                }
                flushFlashBlock();
#ifdef HASH_TABLE
                rebuildHashtable();
#endif
                error = 0;
                break;
            }
        }
        if (error) {
            return CMDERR_TOO_MANY_EVENTS;
        }
    }
 
    if (writeEv(tableIndex, evNum, evVal)) {
        // failed to write
        return CMDERR_INV_EV_IDX;
    }
    // success
    flushFlashBlock();
#ifdef HASH_TABLE
    rebuildHashtable();
#endif
    return 0;
}

/**
 * Find an event in the EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_ and return it's index.
 * 
 * @param nodeNumber
 * @param eventNumber
 * @return index into EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_ or NO_INDEX if not present
 */
uint8_t findEvent(uint16_t nodeNumber, uint16_t eventNumber) {
#ifdef HASH_TABLE
    uint8_t hash = getHash(nodeNumber, eventNumber);
    uint8_t chainIdx;
    for (chainIdx=0; chainIdx<CHAIN_LENGTH; chainIdx++) {
        uint8_t tableIndex = eventChains[hash][chainIdx];
        uint16_t nodeNumber, en;
        if (tableIndex == NO_INDEX) return NO_INDEX;
        nodeNumber = getNN(tableIndex);
        en = getEN(tableIndex);
        if ((nn == nodeNumber) && (en == eventNumber)) {
            return tableIndex;
        }
    }
#else
    uint8_t tableIndex;
    for (tableIndex=0; tableIndex < NUM_EVENTS; tableIndex++) {
        EventTableFlags f;
        f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
        if (( ! f.freeEntry) && ( ! f.continuation)) {
            uint16_t node, en;
            node = getNN(tableIndex);
            en = getEN(tableIndex);
            if ((node == nodeNumber) && (en == eventNumber)) {
                return tableIndex;
            }
        }
    }
#endif
    return NO_INDEX;
}

/**
 * Write an EV value to an event.
 * 
 * @param tableIndex the index into the event table
 * @param evNum the EV number (0 for the produced action)
 * @param evVal
 * @return 0 if success otherwise the error
 */
uint8_t writeEv(uint8_t tableIndex, uint8_t evNum, uint8_t evVal) {
    EventTableFlags f;
    uint8_t startIndex = tableIndex;
    if (evNum >= PARAM_NUM_EV_EVENT) {
        return CMDERR_INV_EV_IDX;
    }
    while (evNum >= EVENT_TABLE_WIDTH) {
        uint8_t nextIdx;
        
        // skip forward looking for the right chained table entry
        evNum -= EVENT_TABLE_WIDTH;
        f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
        
        if (f.continued) {
            tableIndex = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NEXT);
            if (tableIndex == NO_INDEX) {
                return CMDERR_INVALID_EVENT;
            }
        } else {
            // Ian - 2k check for special case. Don't create an entry for a NO_ACTION
            // This is a solution to the problem of FCU filling the event table with unused
            // 00 Actions. 
            // It does not fix a much less frequent problem of releasing some of the 
            // table entries when they are filled with No Action.
            // don't add a new table slot just to store a NO_ACTION
            if (evVal == EV_FILL) {
                return 0;
            }
            // find the next free entry
            for (nextIdx = tableIndex+1 ; nextIdx < NUM_EVENTS; nextIdx++) {
                EventTableFlags nextF;
                nextF.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*nextIdx+EVENTTABLE_OFFSET_FLAGS);
                if (nextF.freeEntry) {
                    uint8_t e;
                     // found a free slot, initialise it
                    writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*nextIdx+EVENTTABLE_OFFSET_NN, 0xff); // this field not used
                    writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*nextIdx+EVENTTABLE_OFFSET_NN+1, 0xff); // this field not used
                    writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*nextIdx+EVENTTABLE_OFFSET_EN, 0xff); // this field not used
                    writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*nextIdx+EVENTTABLE_OFFSET_EN+1, 0xff); // this field not used
                    writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*nextIdx+EVENTTABLE_OFFSET_FLAGS, 0x20);    // set continuation flag, clear free and numEV to 0
                    for (e = 0; e < EVENT_TABLE_WIDTH; e++) {
                        writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*nextIdx+EVENTTABLE_OFFSET_EVS+e, EV_FILL); // clear the EVs
                    }
                    // set the next of the previous in chain
                    writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NEXT, nextIdx);
                    // set the continued flag
                    f.continued = 1;
                    writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS, f.asByte);
                    tableIndex = nextIdx;
                    break;
                }
            }
            if (nextIdx >= NUM_EVENTS) {
                // ran out of table entries
                return CMDERR_TOO_MANY_EVENTS;
            }
        } 
    }
    // now write the EV
    writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_EVS+evNum, evVal);
    // update the number per row count
    f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
    if (f.eVsUsed <= evNum) {
        f.eVsUsed = evNum+1U;
        writeNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS, f.asByte);
    }
    // If we are deleting then see if we can remove all
    if (evVal == EV_FILL) {
        checkRemoveTableEntry(startIndex);
    }
    return 0;
}
 
/**
 * Return an EV value for an event.
 * 
 * @param tableIndex the index of the start of an event
 * @param evNum ev number starts at 0 (produced)
 * @return the ev value or -error code if error
 */
int getEv(uint8_t tableIndex, uint8_t evNum) {
    EventTableFlags f;
    if ( ! validStart(tableIndex)) {
        // not a valid start
        return -CMDERR_INVALID_EVENT;
    }
    if (evNum >= PARAM_NUM_EV_EVENT) {
        return -CMDERR_INV_EV_IDX;
    }
    f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
    while (evNum >= EVENT_TABLE_WIDTH) {
        // if evNum is beyond current EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_ entry move to next one
        if (! f.continued) {
            return -CMDERR_NO_EV;
        }
        tableIndex = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NEXT);
        if (tableIndex == NO_INDEX) {
            return -CMDERR_INVALID_EVENT;
        }
        f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
        evNum -= EVENT_TABLE_WIDTH;
    }
    if (evNum+1 > f.eVsUsed) {
        return -CMDERR_NO_EV;
    }
    // it is within this entry
    return (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_EVS+evNum);
}

/**
 * Return the number of EVs for an event.
 * 
 * @param tableIndex the index of the start of an event
 * @return the number of EVs
 */
uint8_t numEv(uint8_t tableIndex) {
    EventTableFlags f;
    uint8_t num=0;
    if ( ! validStart(tableIndex)) {
        // not a valid start
        return 0;
    }
    f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
    while (f.continued) {
        tableIndex = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NEXT);
        if (tableIndex == NO_INDEX) {
            return 0;
        }
        f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
        num += EVENT_TABLE_WIDTH;
    }
    num += f.eVsUsed;
    return num;
}

/**
 * Return all the EV values for an event. EVs are put into the global evs array.
 * 
 * @param tableIndex the index of the start of an event
 * @return the error code or 0 for no error
 */
uint8_t evs[PARAM_NUM_EV_EVENT];
uint8_t getEVs(uint8_t tableIndex) {
    EventTableFlags f;
    uint8_t evNum;
    
    if ( ! validStart(tableIndex)) {
        // not a valid start
        return CMDERR_INVALID_EVENT;
    }
    for (evNum=0; evNum < PARAM_NUM_EV_EVENT; ) {
        uint8_t evIdx;
        for (evIdx=0; evIdx < EVENT_TABLE_WIDTH; evIdx++) {
            evs[evNum] = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_EVS+evIdx);
            evNum++;
        }
        f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
        if (! f.continued) {
            for (; evNum < PARAM_NUM_EV_EVENT; evNum++) {
                evs[evNum] = EV_FILL;
            }
            return 0;
        }
        tableIndex = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NEXT);
        if (tableIndex == NO_INDEX) {
            return CMDERR_INVALID_EVENT;
        }
    }
    return 0;
}

/**
 * Return the NN for an event.
 * Getter so that the application code can obtain information about the event.
 * 
 * @param tableIndex the index of the start of an event
 * @return the Node Number
 */
uint16_t getNN(uint8_t tableIndex) {
    uint16_t hi;
    uint16_t lo;
    EventTableFlags f;
    
    f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
    if (f.forceOwnNN) {
        return nn.word;
    }
    lo = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NN);
    hi = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_NN+1);
    return lo | (hi << 8);
}

/**
 * Return the EN for an event.
 * Getter so that the application code can obtain information about the event.
 * 
 * @param tableIndex the index of the start of an event
 * @return the Event Number
 */
uint16_t getEN(uint8_t tableIndex) {
    uint16_t hi;
    uint16_t lo;
    
    lo = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_EN);
    hi = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_EN+1);
    return lo | (hi << 8);
}

/**
 * Convert an evtIdx from CBUS to an index into the EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_.
 * The CBUS spec uses "EN#" as an index into an "Event Table". This is very implementation
 * specific. In this implementation we do actually have an event table behind the scenes
 * so we can have an EN#. However we may also wish to provide some kind of mapping between
 * the CBUS index and out actual implementation specific index. These functions allow us
 * to have a mapping.
 * I currently I just adjust by 1 since the CBUS index starts at 1 whilst the EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_
 * index starts at 0.
 * 
 * @param evtIdx
 * @return an index into EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_
 */
uint8_t evtIdxToTableIndex(uint8_t evtIdx) {
    return evtIdx-1U;
}

/**
 * Convert an internal tableIndex into a CBUS EvtIdx.
 * 
 * @param tableIndex index into the EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*i+EVENTTABLE_OFFSET_
 * @return an CBUS EvtIdx
 */
uint8_t tableIndexToEvtIdx(uint8_t tableIndex) {
    return tableIndex+1U;
}


/**
 * Check to see if any event entries can be removed.
 * 
 * @param tableIndex
 */
void checkRemoveTableEntry(uint8_t tableIndex) {
    uint8_t e;
    
    if ( validStart(tableIndex)) {
        if (getEVs(tableIndex)) {
            return;
        }
        for (e=0; e<PARAM_NUM_EV_EVENT; e++) {
            if (evs[e] != EV_FILL) {
                return;
            }
        }
        removeTableEntry(tableIndex);
    }
}

/**
 * Checks if the specified index is the start of a set of linked entries.
 * 
 * @param tableIndex the index into eventtable to check
 * @return true if the specified index is the start of a linked set
 */
uint8_t validStart(uint8_t tableIndex) {
    EventTableFlags f;
#ifdef SAFETY
    if (tableIndex >= NUM_EVENTS) return FALSE;
#endif
    f.asByte = (uint8_t)readNVM(EVENT_TABLE_NVM_TYPE, EVENT_TABLE_ADDRESS + EVENTTABLE_ROW_WIDTH*tableIndex+EVENTTABLE_OFFSET_FLAGS);
    if (( !f.freeEntry) && ( ! f.continuation)) {
        return 1;
    } else {
        return 0;
    }
}