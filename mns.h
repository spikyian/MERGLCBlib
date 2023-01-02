#ifndef _MNS_H_
/**
 * @copyright Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
 */
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
 */
/**
 * @author Ian Hogg 
 * @date Dec 2022
 * 
 */ 
#define _MNS_H_
#include "merglcb.h"
#include "module.h"
/**
 *
 * @file
 * Implementation of the MERGLCB Minimum Module Service.
 * @details
 * MNS provides functionality required by all MERGLCB modules.
 *
 * The service definition object is called mnsService.
 * This is a large service as it handles mode transitions, including setting of 
 * node number. The module's LEDs and push button are also supported.
 * Service discovery and Diagnostics are also processed from this service.
 */


/*
 * Expose the service descriptor for MNS.
 */
extern const Service mnsService;

/* The list of the diagnostics supported */
#define NUM_MNS_DIAGNOSTICS 6   ///< The number of diagnostic values for this service
#define MNS_DIAGNOSTICS_ALL         0x00    ///< The a series of DGN messages for each services? supported data.
#define MNS_DIAGNOSTICS_STATUS      0x00    ///< The Global status Byte.
#define MNS_DIAGNOSTICS_UPTIMEH     0x01    ///< The uptime upper word.
#define MNS_DIAGNOSTICS_UPTIMEL     0x02    ///< The uptime lower word.
#define MNS_DIAGNOSTICS_MEMERRS     0x03    ///< The memory status.
#define MNS_DIAGNOSTICS_NNCHANGE    0x04    ///< The number of Node Number changes.
#define MNS_DIAGNOSTICS_RXMESS      0x05    ///< The number of received messages acted upon.

/*
 * The module's node number.
 */
extern Word nn;
/*
 * the module's mode.
 */
extern uint8_t mode;

/*
 * MNS diagnostics
 */
extern DiagnosticVal mnsDiagnostics[NUM_MNS_DIAGNOSTICS];

extern void updateModuleErrorStatus(void);

/**
 * The status of the module's LEDs.
 */
typedef enum {
    OFF,            ///< fixed OFF
    ON,             ///< fixed ON
    FLASH_50_1HZ,   ///< 50% duty cycle  1Hz
    FLASH_50_HALF_HZ,   ///< 50% duty cycle 0.5Hz
    SINGLE_FLICKER_OFF, ///< 250ms pulse off
    SINGLE_FLICKER_ON,  ///< 250ms pulse on
    LONG_FLICKER_OFF,   ///< 500ms pulse off
    LONG_FLICKER_ON     ///< 500ms pulse on
} LedState;

// other externs
extern LedState    ledState[NUM_LEDS];

// LED identifiers
#define GREEN_LED   0
#define YELLOW_LED  1

#endif