#ifndef _MNS_H_
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
#define _MNS_H_
#include "merglcb.h"
/*
 * The MNS service provides the functionality as specified by the MERGLCB 
 * Minimum Node Specification.
 * All MERGLCB modules should implement this service.
 */

/**
 * Expose the service descriptor for MNS.
 */
extern const Service mnsService;

/**
 * Expose the MNS factoryReset function. Clears the Node number and mode.
 */
extern void mnsFactoryReset(void);

/**
 * Expose the MNS powerUp function. Loads the NN and mode from NVM if the store 
 * is valid. Initialises module functionality and clears the diagnostic counters.
 */
extern void mnsPowerUp(void);

/**
 * Expose the MNS poll function. Used to flash the LEDs and to
 * control the mode transition timeouts.
 */
extern void mnsPoll(void);

/**
 * Expose the MNS processMessage function. Handles all the MNS opcodes
 */
extern uint8_t mnsProcessMessage(Message * m);

/**
 * Handles the tickTime interrupt.
 */
extern void mnsLowIsr(void);

/**
 * Allows the MNS diagnostic data to be retrieved.
 * @param index
 * @return 
 */
extern DiagnosticVal * mnsGetDiagnostic(uint8_t index);
/* The list of the diagnostics supported */
#define NUM_MNS_DIAGNOSTICS 7
#define MNS_DIAGNOSTICS_ALL         0x00    // return a series of DGN messages for each services? supported data.
#define MNS_DIAGNOSTICS_STATUS      0x01    // return Global status Byte.
#define MNS_DIAGNOSTICS_UPTIMEH     0x02    // return uptime upper word.
#define MNS_DIAGNOSTICS_UPTIMEL     0x03    // return uptime lower word.
#define MNS_DIAGNOSTICS_MEMSTAT     0x04    // return memory status.
#define MNS_DIAGNOSTICS_NNCHANGE    0x05    // return number of Node Number changes.
#define MNS_DIAGNOSTICS_ERRSTAT     0x06    // return soft error status.
#define MNS_DIAGNOSTICS_RXMESS      0x07    // return number of received messages acted upon.

/**
 * The module's node number.
 */
extern Word nn;
/**
 * the module's mode.
 */
extern uint8_t mode;

/**
 * The status of the module's LEDs.
 */
typedef enum {
    OFF,            // fixed OFF
    ON,             // fixed ON
    FLASH_50_1HZ,   // 50% duty cycle 
    FLASH_50_HALF_HZ,   //
    SINGLE_FLICKER_OFF,
    SINGLE_FLICKER_ON
} LedState;

// LED identifiers
#define YELLOW_LED  0
#define GREEN_LED   1

#endif