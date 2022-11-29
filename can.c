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

/*
 * Implementation of the MERGLCB CAN service. Uses Controller Area Network to
 * carry MERGLCB messages.
 */
#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "can.h"
#include "mns.h"

#include "romops.h"

/* The CAN service descriptor */
const Service canService = {
    SERVICE_ID_CAN,     // id
    1,                  // version
    canFactoryReset,    // factoryReset
    canPowerUp,         // powerUp
    canProcessMessage,  // processMessage
    canPoll,            // poll
    canIsr,             // highIsr
    NULL,               // lowIsr
    canGetDiagnostic    // getDiagnostic
};

/**
 * The CANID.
 */
static uint8_t canId;
/**
 * The set of diagnostics for the CAN service
 */
static DiagnosticVal canDiagnostics[NUM_CAN_DIAGNOSTICS];

// forward declarations
uint8_t messageAvailable(void);

//CAN SERVICE

/**
 * Perform the CAN factory reset. Just set the CANID to the default of 1.
 */
void canFactoryReset(void) {
    canId = 1;
    writeNVM(CANID_NVM_TYPE, CANID_ADDRESS, canId);
}

/**
 * Do the CAN power up. Get the saved CANID, provision the ECAN peripheral 
 * of the PIC and set the buffers up.
 */
void canPowerUp(void) {
    int temp;
    // initialise the CAN peripheral
    
    temp = readNVM(CANID_NVM_TYPE, CANID_ADDRESS);
    if (temp < 0) {
        // Unsure what to do
        canId = CANID_DEFAULT;
    } else {
        canId = (uint8_t)temp;
    }
}

// a bit of shorthand
#define CAN_NNHI     bytes[0]
#define CAN_NNLO     bytes[1]

/**
 * Process the CAN specific MERGLCB messages. The MERGLCB CAN specification
 * describes two opcodes ENUM and CANID but this implementation does not 
 * require these as both are handled automatically.
 * @param m
 * @return 
 */
uint8_t canProcessMessage(Message * m) {
    // check NN matches us
    if (m->CAN_NNHI != nn.bytes.hi) return 0;
    if (m->CAN_NNLO != nn.bytes.lo) return 0;
    
    // Handle any CAN specific OPCs
    switch (m->opc) {
        case OPC_ENUM:
        case OPC_CANID:
            return 1;
    }
    return 0;
}

/**
 * Poll the CAN interface. Not yet sure what needs to be done.
 */
void canPoll(void) {
    Message * m;
    if (messageAvailable()) {
        m = getReceiveBuffer();
        if (m == NULL) {
            // No RX buffer available
        } else {
            // copy message into buffer
            processMessage(m);
        }
    }
}

/**
 * Handle the interrupts from the CAN peripheral. 
 */
void canIsr(void) {
    // If RX then transfer frame from CAN peripheral to RX message buffer
    // handle enumeration frame
    // check for CANID clash and start self enumeration process
    
    // If TX then transfer next frame from TX buffer to CAN peripheral 
    
}

/**
 * Provide the means to return the diagnostic data.
 * @param index the diagnostic index
 * @return a pointer to the diagnostic data or NULL if the data isn't available
 */
DiagnosticVal * canGetDiagnostic(uint8_t index) {
    if ((index<1) || (index>NUM_CAN_DIAGNOSTICS)) {
        return NULL;
    }
    return &(canDiagnostics[index-1]);
}

/**
 * Dunno yet.
 * @return 1 if a message is available, 0 otherwise
 */
uint8_t messageAvailable(void) {
    // TODO     check to see if CAN frame available
    return 1;
}