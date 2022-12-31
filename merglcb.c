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
#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "romops.h"
#include "hardware.h"
#include "ticktime.h"
#include "timedResponse.h"
#include "mns.h"

/** The developer of an application must provide a module.h which has 
 * module/application specific definitions which influence the behaviour of MERGLCB.
 * 
 * The application source file must provide:
 * 1. an array of Service definitions
 *      const Service * const services[]
 *    which must be initialised with pointers to each service definition for the
 *    services required by the application.
 * 2. a void init(void) function which sets the transpoint pointer e.g.
 *     transport = &canTransport;
 *    This function must also do any additional, application specific initialisation.
 * 3. a void loop(void) function to perform any regular processing required by
 *    the application.
 * 3. Functions required by the MERGLCB library depending upon the services used:
 *      uint8_t APP_nvDefault(uint8_t index)
 *      void APP_nvValueChanged(uint8_t index, uint8_t value, uint8_t oldValue)
 *      NvValidation APP_nvValidate(uint8_t index, uint8_t value)
 *      uint8_t APP_addEvent(uint16_t nodeNumber, uint16_t eventNumber, uint8_t evNum, uint8_t evVal)
 *      ValidTime APP_isSuitableTimeToWriteFlash(void)
 *      Processed APP_preProcessMessage(Message * m)
 *      Processed APP_postProcessMessage(Message * m) 
 * 
 * 
 * DEFINITIONS FOR MNS SERVICE
 * #define NUM_SERVICES to be the number of services implemented by the module.
 *                      The application must put the services into the services array.

 * #define APP_NVM_VERSION the version number of the data structures stored in NVM
 *                      this is located where NV#0 is stored therefore NV_ADDRESS
 *                      and NV_NVM_TYPE must be defined even without the NV service.
 * 
 * 
 * DEFINITIONS FOR NV SERVICE
 * #define NV_NUM       The number of NVs. Will be returned in the parameter block.
 * #define NV_CACHE     Defined, as opposed to undefined, to enable a cache of
 *                      NVs in RAM. Uses more RAM but speeds up access to NVs when
 *                      in normal operation processing events. 
 * #define NV_ADDRESS   the address in non volatile memory to place the NVM version 
 *                      number and NVs if the NV service is included.
 * #define NV_NVM_TYPE  the type of NVM memory to be used for NVs. Can be either
 *                      EEPROM_NVM_TYPE or FLASH_NVM_TYPE.
 * Function uint8_t APP_nvDefault(uint8_t index) The application must implement this 
 *                      function to provide factory default values for NVs.
 * Function NvValidation APP_nvValidate(uint8_t index, uint8_t value) The application
 *                      must implement this function in order to validate that
 *                      the value being written to an NV is valid.
 * Function void APP_nvValueChanged(uint8_t index, uint8_t newValue, uint8_t oldValue)
 *                      The application must implement this function in order to
 *                      perform any needed functionality to be performed when an 
 *                      NV value is changed.
 * 
 * 
 * DEFINITIONS FOR CAN SERVICE
 * #define CANID_ADDRESS The address of the CANID. PIC modules normally stored 
 *                      this at TOP-1.
 * #define CANID_NVM_TYPE The type of NVM where to store the CANID. This can be
 *                      set to be either EEPROM_NVM_TYPE or FLASH_NVM_TYPE. The
 *                      PIC modules normally have this set to EEPROM_NVM_TYPE.
 * #define CAN_INTERRUPT_PRIORITY 0 for low priority, 1 for high priotity
 * #define CAN_NUM_RXBUFFERS the number of receive message buffers to be created. A
 *                      larger number of buffers will reduce the chance of missing
 *                      messages but will need to be balanced with the amount of 
 *                      RAM available.
 * #define CAN_NUM_TXBUFFERS the number of transmit buffers to be created. Fewer 
 *                      transmit buffers will be needed then receive buffers, 
 *                      the timedResponse mechanism means that 4 or fewer buffers
 *                      should be sufficient.
 * 
 * 
 * DEFINITIONS FOR BOOT SERVICE
 * #define BOOT_FLAG_ADDRESS This should be set to where the module's bootloader
 *                      places the bootflag.
 * #define BOOT_FLAG_NVM_TYPE This should be set to be the type of NVM where the
 *                      bootloader stores the boot flag. This can be set to be 
 *                      either EEPROM_NVM_TYPE or FLASH_NVM_TYPE. The PIC
 *                      modules normally have this set to EEPROM_NVM_TYPE.
 * #define BOOTLOADER_PRESENT The module should define, as opposed to undefine, 
 *                      this to indicate that the application should be 
 *                      compiled to start at 0x800 to allow room for the bootloader 
 *                      between 0x000 and 0x7FF.
 * 
 * 
 * DEFINITIONS FOR NMS SERVICE
 * #define clkMHz       Must be set to the clock speed of the module. Typically 
 *                      this would be 4 or 16.
 * #define NN_ADDRESS   This must be set to the address in non volatile memory
 *                      at which the node number is to be stored.
 * #define NN_NVM_TYPE  This must be set to the type of the NVM where the node
 *                      number is to be stored.
 * #define MODE_ADDRESS This must be set to the address in non volatile memory
 *                      at which the mode variable is to be stored.
 * #define MODE_NVM_TYPE This must be set to the type of the NVM where the mode
 *                      variable is to be stored.
 * #define NUM_LEDS     The application must set this to either 1 or 2 to 
 *                      indicate the number of LEDs on the module for indicating
 *                      operating mode.
 * #define APP_setPortDirections() This macro must be set to configure the 
 *                      processor's pins for output to the LEDs and input from the
 *                      push button. It should also enable digital I/O if required 
 *                      by the processor.
 * #define APP_writeLED1(state) This macro must be defined to set LED1 (normally
 *                      yellow) to the state specified. 1 is LED on.
 * #define APP_writeLED2(state) This macro must be defined to set LED1 (normally
 *                      green) to the state specified. 1 is LED on.
 * #define APP_pbState() This macro must be defined to read the push button
 *                      input, returning true when the push button is held down.
 * #define NAME         The name of the module must be defined. Must be exactly 
 *                      7 characters. Shorter names should be padded on the right 
 *                      with spaces. The name must leave off the communications 
 *                      protocol e.g. the CANMIO module would be set to "MIO    ".
 * 
 * The following parameter values are required to be defined by MNS:
 * #define PARAM_MANU              See the manufacturer settings in merglcb.h
 * #define PARAM_MAJOR_VERSION     The major version number
 * #define PARAM_MINOR_VERSION     The minor version character. E.g. 'a'
 * #define PARAM_BUILD_VERSION     The build version number
 * #define PARAM_MODULE_ID         The module ID. Normally set to MTYP_MERGLCB
 * #define PARAM_NUM_NV            The number of NVs. Normally set to NV_NUM
 * #define PARAM_NUM_EVENTS        The number of events.
 * #define PARAM_NUM_EV_EVENT      The number of EVs per event
 * 
 * 
 * 
 * DEFINITIONS FOR EVENT TEACH SERVICE
 * #define EVENT_TABLE_WIDTH   This the the width of the table - not the 
 *                       number of EVs per event as multiple rows in
 *                       the table can be used to store an event.
 * #define NUM_EVENTS          The number of rows in the event table. The
 *                        actual number of events may be less than this
 *                        if any events use more the 1 row.
 * #define EVENT_TABLE_ADDRESS   The address where the event table is stored. 
 * #define EVENT_TABLE_NVM_TYPE  Set to be either FLASH_NVM_TYPE or EEPROM_NVM_TYPE
 * #define EVENT_HASH_TABLE      If defined then hash tables will be used for
 *                        quicker lookup of events at the expense of additional RAM.
 * #define EVENT_HASH_LENGTH     If hash tables are used then this sets the length
 *                        of the hash.
 * #define EVENT_CHAIN_LENGTH    If hash tables are used then this sets the number
 *                        of events in the hash chain.
 * #define MAX_HAPPENING         Set to be the maximum Happening value
 * #define PRODUCED_EVENTS       define if the event producer service is enabled.
 * #define CONSUMED_EVENTS       define if the event consumer service is enabled.
 * 
 * 
 * DEFINITIONS FOR THE EVENT PRODUCER SERVICE
 * #define HAPPENING_SIZE        Set to the number of bytes to hold a Happening.
 *                               Can be either 1 or 2.
 * 
 * 
 * DEFINITIONS FOR THE EVENT CONSUMER SERVICE
 * #define HANDLE_DATA_EVENTS    Define if the ACON1/ACON2/ACON3 style events 
 *                               with data are to used in the same way as ACON 
 *                               style events.
 * #define COMSUMER_EVS_AS_ACTIONS Define if the EVs are to be treated to be Actions
 * #define ACTION_SIZE           The number of bytes used to hold an Action. 
 *                               Currently must be 1.
 * #define ACTION_QUEUE_SIZE     The size of the Action queue.
 * 
 * 
 *************************************************************************
 * 
 * The structure of the library:
 * 
 * -----------
 * |   APP   |
 * |     ------------------------------------------------------------------
 * |     |                            MERGLCB                             |
 * |     |                                                                |
 * |     ----------------------- ------------------------------------------
 * |         |  |   Service Interface      | |    Transport Interface     |
 * |         |  ---------------------------- ------------------------------
 * |         |  | service | | service | |          Transport service      |
 * -----------  |         | |         | |                                 |
 *              ----------- ----------- -----------------------------------
 * 
 * All services must support the Service interface.
 * Transport services must also support the Transport interface.
 * The APP makes use of MERGLCB functionality and also provides functionality to
 * MERGLCB. 
 * 
 */



/*
 * Here is the list of the priorities for each opcode.
 */
// message priority
const Priority priorities[256] = {
    pNORMAL,   // OPC_ACK=0x00,
    pNORMAL,   // OPC_NAK=0x01,
    pHIGH,   // OPC_HLT=0x02,
    pABOVE,   // OPC_BON=0x03,
    pABOVE,   // OPC_TOF=0x04,
    pABOVE,   // OPC_TON=0x05,
    pABOVE,   // OPC_ESTOP=0x06,
    pHIGH,   // OPC_ARST=0x07,
    pABOVE,   // OPC_RTOF=0x08,
    pABOVE,   // OPC_RTON=0x09,
    pHIGH,   // OPC_RESTP=0x0A,
    pNORMAL,   // OPC_RSTAT=0x0C,
    pLOW,   // OPC_QNN=0x0D,
    pLOW,   // OPC_RQNP=0x10,
    pNORMAL,   // OPC_RQMN=0x11,
            pNORMAL,    // 0x12
            pNORMAL,    // 0x13
            pNORMAL,    // 0x14
            pNORMAL,    // 0x15
            pNORMAL,    // 0x16
            pNORMAL,    // 0x17
            pNORMAL,    // 0x18
            pNORMAL,    // 0x19
            pNORMAL,    // 0x1A
            pNORMAL,    // 0x1B
            pNORMAL,    // 0x1C
            pNORMAL,    // 0x1D
            pNORMAL,    // 0x1E
            pNORMAL,    // 0x1F
            pNORMAL,    // 0x20
    pNORMAL,   // OPC_KLOC=0x21,
    pNORMAL,   // OPC_QLOC=0x22,
    pNORMAL,   // OPC_DKEEP=0x23,
            pNORMAL,    // 0x24
            pNORMAL,    // 0x25
            pNORMAL,    // 0x26
            pNORMAL,    // 0x27
            pNORMAL,    // 0x28
            pNORMAL,    // 0x29
            pNORMAL,    // 0x2A
            pNORMAL,    // 0x2B
            pNORMAL,    // 0x2C
            pNORMAL,    // 0x2D
            pNORMAL,    // 0x2E
            pNORMAL,    // 0x2F
    pNORMAL,   // OPC_DBG1=0x30,
            pNORMAL,    // 0x31
            pNORMAL,    // 0x32
            pNORMAL,    // 0x33
            pNORMAL,    // 0x34
            pNORMAL,    // 0x35
            pNORMAL,    // 0x36
            pNORMAL,    // 0x37
            pNORMAL,    // 0x38
            pNORMAL,    // 0x39
            pNORMAL,    // 0x3A
            pNORMAL,    // 0x3B
            pNORMAL,    // 0x3C
            pNORMAL,    // 0x3D
            pNORMAL,    // 0x3E
    pNORMAL,    // OPC_EXTC=0x3F
    pNORMAL,   // OPC_RLOC=0x40,
    pNORMAL,   // OPC_QCON=0x41,
    pLOW,   // OPC_SNN=0x42,
    pNORMAL,   // OPC_ALOC=0x43,
    pNORMAL,   // OPC_STMOD=0x44,
    pNORMAL,   // OPC_PCON=0x45,
    pNORMAL,   // OPC_KCON=0x46,
    pNORMAL,   // OPC_DSPD=0x47,
    pNORMAL,   // OPC_DFLG=0x48,
    pNORMAL,   // OPC_DFNON=0x49,
    pNORMAL,   // OPC_DFNOF=0x4A,
            pNORMAL,    // 0x4B
    pLOW,   // OPC_SSTAT=0x4C,
            pNORMAL,    // 0x4D
            pNORMAL,    // 0x4E
    pLOW,   // OPC_NNRSM=0x4F,
    pLOW,   // OPC_RQNN=0x50,
    pLOW,   // OPC_NNREL=0x51,
    pLOW,   // OPC_NNACK=0x52,
    pLOW,   // OPC_NNLRN=0x53,
    pLOW,   // OPC_NNULN=0x54,
    pLOW,   // OPC_NNCLR=0x55,
    pLOW,   // OPC_NNEVN=0x56,
    pLOW,   // OPC_NERD=0x57,
    pLOW,   // OPC_RQEVN=0x58,
    pLOW,   // OPC_WRACK=0x59,
    pLOW,   // OPC_RQDAT=0x5A,
    pLOW,   // OPC_RQDDS=0x5B,
    pLOW,   // OPC_BOOT=0x5C,
    pLOW,   // OPC_ENUM=0x5D,
    pLOW,   // OPC_NNRST=0x5E,
    pLOW,   // OPC_EXTC1=0x5F,
    pNORMAL,   // OPC_DFUN=0x60,
    pNORMAL,   // OPC_GLOC=0x61,
    pNORMAL,   // OPC_ERR=0x63,
            pNORMAL,    // 0x64
            pNORMAL,    // 0x65
    pHIGH,   // OPC_SQU=0x66,
            pNORMAL,    // 0x67
            pNORMAL,    // 0x68
            pNORMAL,    // 0x69
            pNORMAL,    // 0x6A
            pNORMAL,    // 0x6B
            pNORMAL,    // 0x6C
            pNORMAL,    // 0x6D
            pNORMAL,    // 0x6E
    pLOW,   // OPC_CMDERR=0x6F,
    pLOW,   // OPC_EVNLF=0x70,
    pLOW,   // OPC_NVRD=0x71,
    pLOW,   // OPC_NENRD=0x72,
    pLOW,   // OPC_RQNPN=0x73,
    pLOW,   // OPC_NUMEV=0x74,
    pLOW,   // OPC_CANID=0x75,
    pLOW,   // OPC_MODE=0x76,
            pNORMAL,    // 0x77
    pLOW,   // OPC_RQSD=0x78,
            pNORMAL,    // 0x79
            pNORMAL,    // 0x7A
            pNORMAL,    // 0x7B
            pNORMAL,    // 0x7C
            pNORMAL,    // 0x7D
            pNORMAL,    // 0x7E
    pLOW,   // OPC_EXTC2=0x7F,
    pNORMAL,   // OPC_RDCC3=0x80,
            pNORMAL,    // 0x81
    pNORMAL,   // OPC_WCVO=0x82,
    pNORMAL,   // OPC_WCVB=0x83,
    pNORMAL,   // OPC_QCVS=0x84,
    pNORMAL,   // OPC_PCVS=0x85,
            pNORMAL,    // 0x86
    pLOW,   // OPC_RDGN=0x87,
            pNORMAL,    // 0x88
            pNORMAL,    // 0x89
            pNORMAL,    // 0x8A
            pNORMAL,    // 0x8B
            pNORMAL,    // 0x8C
            pNORMAL,    // 0x8D
    pLOW,   // OPC_NVSETRD=0x8E,
            pNORMAL,    // 0x8F
    pLOW,   // OPC_ACON=0x90,
    pLOW,   // OPC_ACOF=0x91,
    pLOW,   // OPC_AREQ=0x92,
    pLOW,   // OPC_ARON=0x93,
    pLOW,   // OPC_AROF=0x94,
    pLOW,   // OPC_EVULN=0x95,
    pLOW,   // OPC_NVSET=0x96,
    pLOW,   // OPC_NVANS=0x97,
    pLOW,   // OPC_ASON=0x98,
    pLOW,   // OPC_ASOF=0x99,
    pLOW,   // OPC_ASRQ=0x9A,
    pLOW,   // OPC_PARAN=0x9B,
    pLOW,   // OPC_REVAL=0x9C,
    pLOW,   // OPC_ARSON=0x9D,
    pLOW,   // OPC_ARSOF=0x9E,
    pLOW,   // OPC_EXTC3=0x9F
    pNORMAL,   // OPC_RDCC4=0xA0,
            pNORMAL,    // 0xA1
    pNORMAL,   // OPC_WCVS=0xA2,
            pNORMAL,    // 0xA3
            pNORMAL,    // 0xA4
            pNORMAL,    // 0xA5
            pNORMAL,    // 0xA6
            pNORMAL,    // 0xA7
            pNORMAL,    // 0xA8
            pNORMAL,    // 0xA9
            pNORMAL,    // 0xAA
    pLOW,   // OPC_HEARTB=0xAB,
    pLOW,   // OPC_SD=0xAC,
            pNORMAL,    // 0xAD
            pNORMAL,    // 0xAE
    pLOW,   // OPC_GRSP=0xAF,
    pLOW,   // OPC_ACON1=0xB0,
    pLOW,   // OPC_ACOF1=0xB1,
    pLOW,   // OPC_REQEV=0xB2,
    pLOW,   // OPC_ARON1=0xB3,
    pLOW,   // OPC_AROF1=0xB4,
    pLOW,   // OPC_NEVAL=0xB5,
    pLOW,   // OPC_PNN=0xB6,
           pNORMAL,    // 0xB7
    pLOW,   // OPC_ASON1=0xB8,
    pLOW,   // OPC_ASOF1=0xB9,
           pNORMAL,    // 0xBA
           pNORMAL,    // 0xBB
           pNORMAL,    // 0xBC
    pLOW,   // OPC_ARSON1=0xBD,
    pLOW,   // OPC_ARSOF1=0xBE,
    pLOW,   // OPC_EXTC4=0xBF,
    pNORMAL,   // OPC_RDCC5=0xC0,
    pNORMAL,   // OPC_WCVOA=0xC1,
    pNORMAL,   // OPC_CABDAT=0xC2,
           pNORMAL,    // 0xC3
           pNORMAL,    // 0xC4
           pNORMAL,    // 0xC5
           pNORMAL,    // 0xC6
    pLOW,   // OPC_DGN=0xC7,
           pNORMAL,    // 0xC8
           pNORMAL,    // 0xC9
           pNORMAL,    // 0xCA
           pNORMAL,    // 0xCB
           pNORMAL,    // 0xCC
           pNORMAL,    // 0xCD
           pNORMAL,    // 0xCE
    pNORMAL,   // OPC_FCLK=0xCF,
    pLOW,   // OPC_ACON2=0xD0,
    pLOW,   // OPC_ACOF2=0xD1,
    pLOW,   // OPC_EVLRN=0xD2,
    pLOW,   // OPC_EVANS=0xD3,
    pLOW,   // OPC_ARON2=0xD4,
    pLOW,   // OPC_AROF2=0xD5,
           pNORMAL,    // 0xD6
           pNORMAL,    // 0xD7
    pLOW,   // OPC_ASON2=0xD8,
    pLOW,   // OPC_ASOF2=0xD9,
           pNORMAL,    // 0xDA
           pNORMAL,    // 0xDB
           pNORMAL,    // 0xDC
    pLOW,   // OPC_ARSON2=0xDD,
    pLOW,   // OPC_ARSOF2=0xDE,
    pLOW,   // OPC_EXTC5=0xDF,
    pNORMAL,   // OPC_RDCC6=0xE0,
    pNORMAL,   // OPC_PLOC=0xE1,
    pLOW,   // OPC_NAME=0xE2,
    pNORMAL,   // OPC_STAT=0xE3,
           pNORMAL,    // 0xE4
           pNORMAL,    // 0xE5
    pLOW,   // OPC_ENACK=0xE6,
    pLOW,   // OPC_ESD=0xE7,
           pNORMAL,    // 0xE8
    pLOW,   // OPC_ESD=0xE9,
           pNORMAL,    // 0xEA
           pNORMAL,    // 0xEB
           pNORMAL,    // 0xEC
           pNORMAL,    // 0xED
           pNORMAL,    // 0xEE
    pLOW,   // OPC_PARAMS=0xEF,
    pLOW,   // OPC_ACON3=0xF0,
    pLOW,   // OPC_ACOF3=0xF1,
    pLOW,   // OPC_ENRSP=0xF2,
    pLOW,   // OPC_ARON3=0xF3,
    pLOW,   // OPC_AROF3=0xF4,
    pLOW,   // OPC_EVLRNI=0xF5,
    pLOW,   // OPC_ACDAT=0xF6,
    pLOW,   // OPC_ARDAT=0xF7,
    pLOW,   // OPC_ASON3=0xF8,
    pLOW,   // OPC_ASOF3=0xF9,
    pLOW,   // OPC_DDES=0xFA,
    pLOW,   // OPC_DDRS=0xFB,
    pLOW,   // OPC_ARSON3=0xFD,
    pLOW,   // OPC_ARSOF3=0xFE
           pNORMAL,    // 0xFF
};

/*
 * These are some hacks to ensure that the ISRs are located at addresses to
 * give space for the parameter block.
 */
#ifdef __18CXX
void ISRLow(void);
void ISRHigh(void);

void high_irq_errata_fix(void);

/*
 * Interrupt vectors (moved higher when bootloader present)
 */

// High priority interrupt vector

#ifdef BOOTLOADER_PRESENT
    #pragma code high_vector=0x808
#else
    #pragma code high_vector=0x08
#endif


//void interrupt_at_high_vector(void)

void HIGH_INT_VECT(void)
{
    _asm
        CALL high_irq_errata_fix, 1
    _endasm
}

/*
 * See 18F2480 errata
 */
void high_irq_errata_fix(void) {
    _asm
        POP
        GOTO ISRHigh
    _endasm
}

// low priority interrupt vector

#ifdef BOOTLOADER_PRESENT
    #pragma code low_vector=0x818
#else
    #pragma code low_vector=0x18
#endif

void LOW_INT_VECT(void)
{
    _asm GOTO ISRLow _endasm
}
#endif

#ifdef BOOTLOADER_PRESENT
// ensure that the bootflag is zeroed
#pragma romdata BOOTFLAG
uint8_t eeBootFlag = 0;
#endif


/**
 * The module's transport interface.
 */
const Transport * transport;            // pointer to the Transport interface
static Message tmpMessage;

/**
 * Used to control the rate at which timedResponse messages are sent.
 */
static TickValue timedResponseTime;

/** APP externs */
extern Processed APP_preProcessMessage(Message * m);
extern Processed APP_postProcessMessage(Message * m);

// forward declarations
void setup(void);
void loop(void);


/////////////////////////////////////////////
// SERVICE CHECKING FUNCTIONS
/////////////////////////////////////////////
/**
 * Find a service and obtain its service descriptor if it has been used by the
 * module.
 * @param id the service type id
 * @return the service descriptor or NULL if the service is not used by the module.
 */
const Service * findService(uint8_t id) {
    uint8_t i;
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->serviceNo == id)) {
            return services[i];
        }
    }
    return NULL;
}

/**
 * Obtain the index int the services array of the specified service.
 * @param id the service type id
 * @return the index into the services array or -1 if the service is not used by the module.
 */
uint8_t findServiceIndex(uint8_t serviceType) {
    uint8_t i;
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->serviceNo == serviceType)) {
            return i;
        }
    }
    return SERVICE_ID_NONE;
}

/**
 * Tests whether the module used the specified service.
 * @param id the service type id
 * @return 1 if the service is present 0 otherwise
 */
ServicePresent have(uint8_t id) {
    uint8_t i;
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->serviceNo == id)) {
            return PRESENT;
        }
    }
    return NOT_PRESENT;
}

/////////////////////////////////////////////
//FUNCTIONS TO CALL SERVICES
/////////////////////////////////////////////
/**
 * Perform the factory reset for all services and MERGLCB base.
 * MERGLCB function to perform necessary factory reset functionality and also
 * call the factoryReset function for each service.
 */
void factoryReset(void) {
    uint8_t i;
 
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->factoryReset != NULL)) {
            services[i]->factoryReset();
        }
    }
    // now write the version number
    writeNVM(MODE_NVM_TYPE, NV_ADDRESS, APP_NVM_VERSION);
}

/**
 * Perform power up for all services and MERGLCB base.
 * MERGLCB function to perform necessary power up functionality and also
 * call the powerUp function for each service.
 */
void powerUp(void) {
    uint8_t i;
    uint8_t divider;
    
    OSCTUNEbits.PLLEN = 1;      // enable the Phase Locked Loop x4
 
    // Initialise the Tick timer. Uses low priority interrupts
    initTicker(0);
    initTimedResponse();
    
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->powerUp != NULL)) {
            services[i]->powerUp();
        }
    }
}

/**
 * Poll each service.
 * MERGLCB function to perform necessary poll functionality and regularly 
 * poll each service.
 * Polling occurs as frequently as possible. It is the responsibility of the
 * service's poll function to ensure that any actions are performed at the 
 * correct rate, for example by using tickTimeSince(lastTime).
 * This also attempts to obtain a message from transport and use the services
 * to process the message. Will also call back into APP to process message.
 */
void poll(void) {
    uint8_t i;
    Message m;
    uint8_t handled;
    
    /* handle any timed responses */
    if (tickTimeSince(timedResponseTime) > 5*FIVE_MILI_SECOND) {
        pollTimedResponse();
        timedResponseTime.val = tickGet();
    }
    /* call any service polls */
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->poll != NULL)) {
            services[i]->poll();
        }
    }
    
    // Handle any incoming messages from the transport
    handled = 0;
    if (transport != NULL) {
        if (transport->receiveMessage != NULL) {
            if (transport->receiveMessage(&m)) {
                if (m.len > 0) {
#if NUM_LEDS == 1
                    ledState[0] = SINGLE_FLICKER_OFF;
#endif
#if NUM_LEDS == 2
                    ledState[GREEN_LED] = SINGLE_FLICKER_ON;
#endif
                    handled = APP_preProcessMessage(&m); // Call App to check for any opcodes to be handled. 
                    if (handled == 0) {
                        for (i=0; i<NUM_SERVICES; i++) {
                            if ((services[i] != NULL) && (services[i]->processMessage != NULL)) {
                                if (services[i]->processMessage(&m)) {
                                    handled = 1;
                                    break;
                                }
                            }
                        }
                        if (handled == 0) {     // Call App to check for any opcodes to be handled. 
                            handled = APP_postProcessMessage(&m);
                        }
                    }
                }
            }
        }
    }
    if (handled) {
#if NUM_LEDS == 1
        ledState[0] = LONG_FLICKER_OFF;
#endif
#if NUM_LEDS == 2
        ledState[GREEN_LED] = LONG_FLICKER_ON;
#endif
    }
}

/**
 * Call each service's high priority interrupt service routine.
 * MERGLCB function to handle low priority interrupts. A service wanting 
 * to use interrupts should enable the interrupts in hardware and then provide 
 * a lowIsr function. Care must to taken to ensure that the cause of the 
 * interrupt is cleared within the ISR and that minimum time is spent in the 
 * ISR. It is preferable to set a flag within the ISR and then perform longer
 * processing within poll().
 */
void highIsr(void) {
    uint8_t i;
    
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->highIsr != NULL)) {
            services[i]->highIsr();
        }
    }
}

/**
 * Call each service's low priority interrupt service routine.
 * MERGLCB function to handle low priority interrupts. A service wanting 
 * to use interrupts should enable the interrupts in hardware and then provide 
 * a lowIsr function. Care must to taken to ensure that the cause of the 
 * interrupt is cleared within the ISR and that minimum time is spent in the 
 * ISR. It is preferable to set a flag within the ISR and then perform longer
 * processing within poll().
 */
void lowIsr(void) {
    uint8_t i;
    
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->lowIsr != NULL)) {
            services[i]->lowIsr();
        }
    }
}


/**
 * Checks that the required number of message bytes are present.
 * @param m the message to be checked
 * @param needed the number of bytes within the message needed including the opc
 * @return PROCESSED if it is an invalid message and should not be processed further
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

/////////////////////////////////////////////
// Message handling functions
/////////////////////////////////////////////

/**
 * Send a message with just OPC.
 * @param opc opcode
 */
void sendMessage0(Opcode opc){
    sendMessage(opc, 1, 0,0,0,0,0,0,0);
}

/**
 * Send a message with OPC and 1 data byte.
 * @param opc opcode
 * @param data1 data byte
 */
void sendMessage1(Opcode opc, uint8_t data1){
    sendMessage(opc, 2, data1, 0,0,0,0,0,0);
}

/**
 * Send a message with OPC and 2 data bytes.
 * @param opc opcode
 * @param data1 data byte 1
 * @param data2 data byte 2
 */
void sendMessage2(Opcode opc, uint8_t data1, uint8_t data2){
    sendMessage(opc, 3, data1, data2, 0,0,0,0,0);
}

/**
 * Send a message with OPC and 3 data bytes.
 * @param opc opcode
 * @param data1 data byte 1
 * @param data2 data byte 2
 * @param data3 data byte 3
 */
void sendMessage3(Opcode opc, uint8_t data1, uint8_t data2, uint8_t data3) {
    sendMessage(opc, 4, data1, data2, data3, 0,0,0,0);
}

/**
 * Send a message with OPC and 4 data bytes.
 * @param opc opcode
 * @param data1 data byte 1
 * @param data2 data byte 2
 * @param data3 data byte 3
 * @param data4 data byte 4
 */
void sendMessage4(Opcode opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4){
    sendMessage(opc, 5, data1, data2, data3, data4, 0,0,0);
}

/**
 * Send a message with OPC and 5 data bytes.
 * @param opc opcode
 * @param data1 data byte 1
 * @param data2 data byte 2
 * @param data3 data byte 3
 * @param data4 data byte 4
 * @param data5 data byte 5
 */
void sendMessage5(Opcode opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5) {
    sendMessage(opc, 6, data1, data2, data3, data4, data5, 0,0);
}

/**
 * Send a message with OPC and 6 data bytes.
 * @param opc opcode
 * @param data1 data byte 1
 * @param data2 data byte 2
 * @param data3 data byte 3
 * @param data4 data byte 4
 * @param data5 data byte 5
 * @param data6 data byte 6
 */
void sendMessage6(Opcode opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6) {
    sendMessage(opc, 7, data1, data2, data3, data4, data5, data6,0);
}

/**
 * Send a message with OPC and 7 data bytes.
 * @param opc opcode
 * @param data1 data byte 1
 * @param data2 data byte 2
 * @param data3 data byte 3
 * @param data4 data byte 4
 * @param data5 data byte 5
 * @param data6 data byte 6
 * @param data7 data byte 7
 */
void sendMessage7(Opcode opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6, uint8_t data7) {
    sendMessage(opc, 8, data1, data2, data3, data4, data5, data6, data7);
}

/**
 * Send a message of variable length with OPC and up to 7 data bytes.
 * @param opc
 * @param len
 * @param data1
 * @param data2
 * @param data3
 * @param data4
 * @param data5
 * @param data6
 * @param data7
 */
void sendMessage(Opcode opc, uint8_t len, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6, uint8_t data7) {
    tmpMessage.opc = opc;
    tmpMessage.len = len;
    tmpMessage.bytes[0] = data1;
    tmpMessage.bytes[1] = data2;
    tmpMessage.bytes[2] = data3;
    tmpMessage.bytes[3] = data4;
    tmpMessage.bytes[4] = data5;
    tmpMessage.bytes[5] = data6;
    tmpMessage.bytes[6] = data7;
    if (transport != NULL) {
        if (transport->sendMessage != NULL) {
            transport->sendMessage(&tmpMessage);
        }
    }
}

/**
 * This is the MERGLCB application start.
 * Do some initialisation, call the factoryReset() if necessary, call the powerUp()
 * service routines. Then call the user's setup() before entering the main loop
 * which calls the user's loop() and service poll().
 */
void main(void) {
    uint8_t i;
    
#if defined(_PIC18)
    RCONbits.IPEN = 1;  // enable interrupt priority
#endif
     
    // init the romops ready for flash writes
    initRomOps();
    
    if (readNVM(NV_NVM_TYPE, NV_ADDRESS) != APP_NVM_VERSION) {
        factoryReset();
    }
    
    // initialise the services
    powerUp();
    
    // call the application's init 
    setup();
    
    // enable the interrupts and ready to go
    bothEi();   
    while(1) {
        // poll the services as quickly as possible.
        // up to service to ignore the polls it doesn't need.
        poll();
        loop();
    }
}


// Interrupt service routines 
#if defined(_18F26K80)
void __interrupt(high_priority) __section("mainSec") isrHigh() {
#endif
#if defined(_18F26K83)
void __interrupt(high_priority, base(0x0808)) isrHigh() {
#endif

    highIsr();
}

/**
 * Low priority interrupt service handler.
 */
#if defined(_18F26K80)
void __interrupt(low_priority) __section("mainSec") isrLow() {
#endif
#if defined(_18F26K83)
void __interrupt(low_priority, base(0x0808)) isrLow() {
#endif

    lowIsr();
}

 
// CONFIG1L
#pragma config RETEN = OFF      // VREG Sleep Enable bit (Ultra low-power regulator is Disabled (Controlled by REGSLP bit))
#pragma config INTOSCSEL = HIGH // LF-INTOSC Low-power Enable bit (LF-INTOSC in High-power mode during Sleep)
#pragma config SOSCSEL = DIG    // SOSC Power Selection and mode Configuration bits (Digital (SCLKI) mode)
#pragma config XINST = OFF      // Extended Instruction Set (Disabled)

// CONFIG1H
#pragma config FOSC = HS1       // Oscillator (HS oscillator (Medium power, 4 MHz - 16 MHz))
#pragma config PLLCFG = OFF      // PLL x4 Enable bit (Disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor (Disabled)
#pragma config IESO = OFF       // Internal External Oscillator Switch Over Mode (Disabled)

// CONFIG2L
#pragma config PWRTEN = ON      // Power Up Timer (Enabled)
#pragma config BOREN = SBORDIS      // Brown Out Detect (Disabled in hardware, SBOREN disabled)
#pragma config BORV = 0         // Brown-out Reset Voltage bits (3.0V)
#pragma config BORPWR = ZPBORMV // BORMV Power level (ZPBORMV instead of BORMV is selected)

// CONFIG2H
#pragma config WDTEN = OFF      // Watchdog Timer (WDT disabled in hardware; SWDTEN bit disabled)
#pragma config WDTPS = 1048576      // Watchdog Postscaler (1:1048576)

// CONFIG3H
#pragma config CANMX = PORTB    // ECAN Mux bit (ECAN TX and RX pins are located on RB2 and RB3, respectively)
#pragma config MSSPMSK = MSK7   // MSSP address masking (7 Bit address masking mode)
#pragma config MCLRE = ON       // Master Clear Enable (MCLR Enabled, RE3 Disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Overflow Reset (Enabled)
#pragma config BBSIZ = BB1K     // Boot Block Size (1K word Boot Block size)

// CONFIG5L
#pragma config CP0 = OFF        // Code Protect 00800-01FFF (Disabled)
#pragma config CP1 = OFF        // Code Protect 02000-03FFF (Disabled)
#pragma config CP2 = OFF        // Code Protect 04000-05FFF (Disabled)
#pragma config CP3 = OFF        // Code Protect 06000-07FFF (Disabled)

// CONFIG5H
#pragma config CPB = OFF        // Code Protect Boot (Disabled)
#pragma config CPD = OFF        // Data EE Read Protect (Disabled)

// CONFIG6L
#pragma config WRT0 = OFF       // Table Write Protect 00800-01FFF (Disabled)
#pragma config WRT1 = OFF       // Table Write Protect 02000-03FFF (Disabled)
#pragma config WRT2 = OFF       // Table Write Protect 04000-05FFF (Disabled)
#pragma config WRT3 = OFF       // Table Write Protect 06000-07FFF (Disabled)

// CONFIG6H
#pragma config WRTC = OFF       // Config. Write Protect (Disabled)
#pragma config WRTB = OFF       // Table Write Protect Boot (Disabled)
#pragma config WRTD = OFF       // Data EE Write Protect (Disabled)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protect 00800-01FFF (Disabled)
#pragma config EBTR1 = OFF      // Table Read Protect 02000-03FFF (Disabled)
#pragma config EBTR2 = OFF      // Table Read Protect 04000-05FFF (Disabled)
#pragma config EBTR3 = OFF      // Table Read Protect 06000-07FFF (Disabled)

// CONFIG7H
#pragma config EBTRB = OFF      // Table Read Protect Boot (Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

