#ifndef _CAN_H_
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
#define _CAN_H_
#include <xc.h>
#include "merglcb.h"
#include "ticktime.h"

/**
 * The service descriptor for the CAN service.
 */
extern const Service canService;
/**
 The transport descriptor for the CAN service.
 */
extern const Transport canTransport;

/**
 * Function to perform any service specific factory reset functionality.
 * Initialises the CANID in EEPROM.
 */
void canFactoryReset(void);
/**
 * Function to perform any service specific power up functionality.
 * Initialises the CAN hardware, variables and diagnostics.
 */
void canPowerUp(void);
/**
 * Function called on a regular basis.
 */
void canPoll(void);
/**
 * The function implementation to to process the MERGLCB messages.
 * The only message processed is BOOTM. 
 * @param m the MERGLCB message to be processed
 * @return indication of whether the messages was processed by this service.
 */
uint8_t canProcessMessage(Message * m);
/**
 * Handle the CAN interrupt
 */
void canIsr(void);
/**
 * Return the CAN specific diagnostic data.
 * @param index identifies which DiagnosticVal to return
 * @return the requested DiagnosticVal or NULL is the data is not available
 */
extern DiagnosticVal * canGetDiagnostic(uint8_t index);
/**
 * The number of diagnostics stored for this service.
 */
#define NUM_CAN_DIAGNOSTICS 10

extern uint8_t canSendMessage(Message * m);
extern uint8_t canReceiveMessage(Message * m);

/**
 * The default value of the CANID.
 */
#define CANID_DEFAULT       1
#define ENUMERATION_TIMEOUT HUNDRED_MILI_SECOND     // Wait time for enumeration responses before setting canid
#define ENUMERATION_HOLDOFF 2 * HUNDRED_MILI_SECOND // Delay afer receiving conflict before initiating our own self enumeration
#define MAX_CANID           0x7F
#define ENUM_ARRAY_SIZE     (MAX_CANID/8)+1              // Size of array for enumeration results
#define LARB_RETRIES    10                          // Number of retries for lost arbitration
#define CAN_TX_TIMEOUT  ONE_SECOND                  // Time for CAN transmit timeout (will resolve to one second intervals due to timer interrupt period)


#ifdef _PIC18
    #define TXBnIE      PIE5bits.TXBnIE
    #define TXBnIF      PIR5bits.TXBnIF
    #define ERRIE       PIE5bits.ERRIE
    #define ERRIF       PIR5bits.ERRIF
    #define FIFOWMIE    PIE5bits.FIFOWMIE
    #define FIFOWMIF    PIR5bits.FIFOWMIF
    #define RXBnIF      PIR5bits.RXBnIF
    #define IRXIF       PIR5bits.IRXIF
    #define RXBnOVFL    COMSTATbits.RXB1OVFL
#else
    #define TXBnIE      PIE3bits.TXBnIE
    #define TXBnIF      PIR3bits.TXBnIF
    #define ERRIE       PIE3bits.ERRIE
    #define ERRIF       PIR3bits.ERRIF
    #define FIFOWMIE    PIE3bits.FIFOWMIE
    #define FIFOWMIF    PIR3bits.FIFOWMIF
    #define RXBnIF      PIR3bits.RXBnIF
    #define IRXIF       PIR3bits.IRXIF
    #define RXBnOVFL    COMSTATbits.RXBnOVFL
#endif
#endif
