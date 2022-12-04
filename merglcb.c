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
/** Module.h has module/application specific definitions which influence the
 * behaviour of MERGLCB.
 * 
 * The application must provide:
 * 1. a void init(void) function which copies the required service definition
 *      objects into the services array. This function must also do any 
 *      additional, application specific initialisation.
 * 2. a void loop(void) function to perform any regular processing.
 * 3. a module.h file which contains the following:
 * 
 * DEFINITIONS FOR MNS SERVICE
 * #define NUM_SERVICES to be the number of services implemented by the module.
 *                      The application's init() function must put the services
 *                      in the services array.
 * #define NUM_RXBUFFERS the number of receive message buffers to be created. A
 *                      larger number of buffers will reduce the chance of missing
 *                      messages but will need to be balanced with the amount of 
 *                      RAM available.
 * #define NUM_TXBUFFERS the number of transmit buffers to be created. Fewer 
 *                      transmit buffers will be needed then receive buffers, 
 *                      the timedResponse mechanism means that 4 or fewer buffers
 *                      should be sufficient.
 * #define APP_NVM_VERSION the version number of the data structures stored in NVM
 *                      this is located where NV#0 is stored therefore NV_ADDRESS
 *                      and NV_NVM_TYPE must be defined even without the NV service.
 * #define NV_ADDRESS   the address in non volatile memory to place the NVM version 
 *                      number and NVs if the NV service is included.
 * #define NV_NVM_TYPE  the type of NVM memory to be used for NVs. Can be either
 *                      EEPROM_NVM_TYPE or FLASH_NVM_TYPE.
 * Function void APP_GSTOP(void) must be provided, even if empty, to handle a GSTOP
 *                      request.
 * 
 * DEFINITIONS FOR NV SERVICE
 * #define NV_NUM       The number of NVs. Will be returned in the parameter block.
 * #define NV_CACHE     Defined, as opposed to undefined, to enable a cache of
 *                      NVs in RAM. Uses more RAM but speeds up access to NVs when
 *                      in normal operation processing events. 
 * Function void APP_factoryResetNv(void) The application must implement this 
 *                      function to provide factory default values for NVs.
 * Function uint8_t APP_nvValidate(uint8_t index, uint8_t value) The application
 *                      must implement this function in order to validate that
 *                      the value being written to an NV is valid.
 * Function void APP_nvValueChanged(uint8_t index, uint8_t newValue, uint8_t oldValue)
 *                      The application must implement this function in order to
 *                      perform any needed functionality to be performed when an 
 *                      NV value is changed.
 * 
 * DEFINITIONS FOR CAN SERVICE
 * #define CANID_ADDRESS The address of the CANID. PIC modules normally stored 
 *                      this at TOP-1.
 * #define CANID_NVM_TYPE The type of NVM where to store the CANID. This can be
 *                      set to be either EEPROM_NVM_TYPE or FLASH_NVM_TYPE. The
 *                      PIC modules normally have this set to EEPROM_NVM_TYPE.
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
 * The APP makes use of merglcb functionality and also provides functionality to
 * merglcb. 
 * 
 * 
 */

#include "romops.h"
#include "ticktime.h"
#include "timedResponse.h"

/*
 * Timer 0 is used for Tick time
 */


/**
 * The list of services implemented by the module.
 */
const Service * services[NUM_SERVICES];   // Services stored in Flash

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
extern uint8_t APP_processMessage(Message * m);

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
int16_t findServiceIndex(uint8_t id) {
    uint8_t i;
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->serviceNo == id)) {
            return i;
        }
    }
    return -1;
}

/**
 * Tests whether the module used the specified service.
 * @param id the service type id
 * @return 1 if the service is present 0 otherwise
 */
uint8_t have(uint8_t id) {
    uint8_t i;
    for (i=0; i<NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->serviceNo == id)) {
            return 1;
        }
    }
    return 0;
}

/////////////////////////////////////////////
//FUNCTIONS TO CALL SERVICES
/////////////////////////////////////////////
/**
 * Perform the factory reset for all services and merglcb base.
 * Merglcb function to perform necessary factory reset functionality and also
 * call the factoryReset function for each service.
 */
void factoryReset(void) {
    uint8_t i;
 
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->factoryReset != NULL)) {
            services[i]->factoryReset();
        }
    }
    // now write the version number
    writeNVM(MODE_NVM_TYPE, NV_ADDRESS, APP_NVM_VERSION);
}

/**
 * Perform power up for all services and merglcb base.
 * Merglcb function to perform necessary power up functionality and also
 * call the powerUp function for each service.
 */
void powerUp(void) {
    uint8_t i;
    uint8_t divider;
    
    OSCTUNEbits.PLLEN = 1;      // enable the Phase Locked Loop x4
 
    // Initialise the Tick timer. Uses low priority interrupts
    initTicker(0);
    initTimedResponse();
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->powerUp != NULL)) {
            services[i]->powerUp();
        }
    }
}

/**
 * Poll each service.
 * Merglcb function to perform necessary poll functionality and regularly 
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
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->poll != NULL)) {
            services[i]->poll();
        }
    }
    
    // Handle any incoming messages from the transport
    handled = 0;
    if (transport != NULL) {
        if (transport->receiveMessage != NULL) {
            if (transport->receiveMessage(&m)) {
                for (i=0; i< NUM_SERVICES; i++) {
                    if ((services[i] != NULL) && (services[i]->processMessage != NULL)) {
                        if (services[i]->processMessage(&m)) {
                            handled = 1;
                            break;
                        };
                    }
                }
            }
        }
    }
    if (handled == 0) {     // TODO check this. May want App to check before services as well as after
        APP_processMessage(&m);
    }
}

/**
 * Call each service's high priority interrupt service routine.
 * Merglcb function to handle low priority interrupts. A service wanting 
 * to use interrupts should enable the interrupts in hardware and then provide 
 * a lowIsr function. Care must to taken to ensure that the cause of the 
 * interrupt is cleared within the Isr and that minimum time is spent in the 
 * Isr. It is preferable to set a flag within the Isr and then perform longer
 * processing within poll().
 */
void highIsr(void) {
    uint8_t i;
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->highIsr != NULL)) {
            services[i]->highIsr();
        }
    }
}

/**
 * Call each service's low priority interrupt service routine.
 * Merglcb function to handle low priority interrupts. A service wanting 
 * to use interrupts should enable the interrupts in hardware and then provide 
 * a lowIsr function. Care must to taken to ensure that the cause of the 
 * interrupt is cleared within the Isr and that minimum time is spent in the 
 * Isr. It is preferable to set a flag within the Isr and then perform longer
 * processing within poll().
 */
void lowIsr(void) {
    uint8_t i;
    
    for (i=0; i< NUM_SERVICES; i++) {
        if ((services[i] != NULL) && (services[i]->lowIsr != NULL)) {
            services[i]->lowIsr();
        }
    }
}

/////////////////////////////////////////////
// Message handling functions
/////////////////////////////////////////////

/**
 * Send a message with just OPC.
 * @param opc opcode
 */
void sendMessage0(uint8_t opc){
    sendMessage(opc, 1, 0,0,0,0,0,0,0);
}

/**
 * Send a message with OPC and 1 data byte.
 * @param opc opcode
 * @param data1 data byte
 */
void sendMessage1(uint8_t opc, uint8_t data1){
    sendMessage(opc, 2, data1, 0,0,0,0,0,0);
}

/**
 * Send a message with OPC and 2 data bytes.
 * @param opc opcode
 * @param data1 data byte 1
 * @param data2 data byte 2
 */
void sendMessage2(uint8_t opc, uint8_t data1, uint8_t data2){
    sendMessage(opc, 3, data1, data2, 0,0,0,0,0);
}

/**
 * Send a message with OPC and 3 data bytes.
 * @param opc opcode
 * @param data1 data byte 1
 * @param data2 data byte 2
 * @param data3 data byte 3
 */
void sendMessage3(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3) {
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
void sendMessage4(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4){
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
void sendMessage5(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5) {
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
void sendMessage6(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6) {
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
void sendMessage7(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6, uint8_t data7) {
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
void sendMessage(uint8_t opc, uint8_t len, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6, uint8_t data7) {
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
