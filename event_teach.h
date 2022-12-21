#ifndef _EVENT_TEACH_H_
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
#define _EVENT_TEACH_H_

#include "xc.h"
#include "module.h"
#include "merglcb.h"

/*
 * EVENT TEACH SERVICE
 * 
 * The events are stored in tables in flash (flash is faster to read than EEPROM).
 * Separate tables are used for Produced events and Consumed events since they require 
 * different data and different lookup schemes.
 * The Produced events are looked up by the action that caused them.
 * The Consumed events are looked up by the CBUS event to find the module specific actions
 * to perform. The lookup is done using a hash table to find the index into the event2Actions table.
 * The action2Event and event2Actions tables are stored in Flash whilst the hashtable
 * lookup for the event2Actions table is stored in RAM.
 * 
 * For Produced events the event is taught using the action stored in the EV field of the
 * CBUS message.
 * For Consumed events the actions are taught using the EV field of the CBUS message.
 * Multiple actions can be specified for a Consumed event. This can be used to set up
 * routes with a single event. 
 *
 * The generic FLiM library code handles the teaching (Learn) and unlearning of events
 * and the storage of the events. The application code just needs to process a consumed 
 * event's actions and to produce actions using the application's actions.
 */

extern const Service eventTeachService;

/**
 * Function called before the EV is saved. This allows the application to perform additional
 * behaviour and to validate that the EV is acceptable.
 * @param nodeNumber
 * @param eventNumber
 * @param evNum
 * @param evVal
 * @return error number or 0 for success
 */
extern uint8_t APP_addEvent(uint16_t nodeNumber, uint16_t eventNumber, uint8_t evNum, uint8_t evVal);

extern Boolean validStart(uint8_t index);
extern int16_t getEv(uint8_t tableIndex, uint8_t evIndex);
extern uint16_t getNN(uint8_t tableIndex);
extern uint16_t getEN(uint8_t tableIndex);
extern uint8_t findEvent(uint16_t nodeNumber, uint16_t eventNumber);
extern uint8_t addEvent(uint16_t nodeNumber, uint16_t eventNumber, uint8_t evNum, uint8_t evVal, uint8_t forceOwnNN);
#ifdef EVENT_HASH_TABLE
extern void rebuildHashtable(void);
extern uint8_t getHash(uint16_t nodeNumber, uint16_t eventNumber);
#endif

#if HAPPENING_SIZE == 2
typedef Word Happening;
#endif
#if HAPPENING_SIZE == 1
typedef uint8_t Happening;
#endif


// A helper structure to store the details of an event.
typedef struct {
    uint16_t NN;
    uint16_t EN;
} Event;

// Indicates whether on ON event or OFF event
typedef enum {
    EVENT_OFF=0,
    EVENT_ON=1
}EventState;


typedef union
{
    struct
    {
        unsigned char eVsUsed:4;  // How many of the EVs in this row are used. Only valid if continued is clear
        uint8_t    continued:1;    // there is another entry 
        uint8_t    continuation:1; // Continuation of previous event entry
        uint8_t    forceOwnNN:1;   // Ignore the specified NN and use module's own NN
        uint8_t    freeEntry:1;    // this row in the table is not used - takes priority over other flags
    };
    uint8_t    asByte;       // Set to 0xFF for free entry, initially set to zero for entry in use, then producer flag set if required.
} EventTableFlags;

typedef struct {
    EventTableFlags flags;          // put first so could potentially use the Event bytes for EVs in subsequent rows.
    uint8_t next;                   // index to continuation also indicates if entry is free
    Event event;                    // the NN and EN
    uint8_t evs[EVENT_TABLE_WIDTH]; // EVENT_TABLE_WIDTH is maximum of 15 as we have 4 bits of maxEvUsed
} EventTable;

#define EVENTTABLE_OFFSET_FLAGS    0
#define EVENTTABLE_OFFSET_NEXT     1
#define EVENTTABLE_OFFSET_NN       2
#define EVENTTABLE_OFFSET_EN       4
#define EVENTTABLE_OFFSET_EVS      6
#define EVENTTABLE_ROW_WIDTH       16

#define NO_INDEX            0xff
#define EV_FILL             0xff

// EVENT DECODING
//    An event opcode has bits 4 and 7 set, bits 1 and 2 clear
//    An ON event opcode also has bit 0 clear
//    An OFF event opcode also has bit 0 set
//
//  eg:
//  ACON  90    1001 0000
//  ACOF  91    1001 0001
//  ASON  98    1001 1000
//  ASOF  99    1001 1001
//
//  ACON1 B0    1011 0000
//  ACOF1 B1    1011 0001
//  ASON1 B8    1011 1000
//  ASOF1 B9    1011 1001
//
//  ACON2 D0    1101 0000
//  ACOF2 D1    1101 0001
//  ASON2 D8    1101 1000
//  ASOF2 D9    1101 1001
//
//  ACON3 F0    1111 0000
//  ACOF3 F1    1111 0001
//  ASON3 F8    1111 1000
//  ASOF3 F9    1111 1001
//
// ON/OFF         determined by d0
// Long/Short     determined by d3
// num data bytes determined by d7,d6
//
#define     EVENT_SET_MASK   0b10010000
#define     EVENT_CLR_MASK   0b00000110
#define     EVENT_ON_MASK    0b00000001
#define     EVENT_SHORT_MASK 0b00001000


#endif