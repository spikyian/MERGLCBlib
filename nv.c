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
 * Implementation of the MERGLCB NV service.
 *
 * The NV service implements the MERGLCB Node Variable Service. This supports 
 * the NVSET, NVRD and NVSETRD opcodes.
 * 
 * If NV_CACHE is defined in module.h then the NV service implements a cache
 * of NV values in RAM. This can be used to speed up obtaining NV values used 
 * within the application at the expense of additional RAM usage.
 */

#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "nv.h"
#include "mns.h"
#include "romops.h"

// service definition
const Service nvService = {
    SERVICE_ID_NV,      // id
    1,                  // version
    nvFactoryReset,     // factoryReset
    nvPowerUp,          // powerUp
    nvProcessMessage,   // processMessage
    NULL,               // poll
    NULL,               // highIsr
    NULL,               // lowIsr
    NULL                // getDiagnostic
};

/**
 *  nv cache
 */
#ifdef NV_CACHE
static uint8_t nvCache[NV_NUM+1];
#endif

/* externs back into APP */
extern uint8_t APP_nvValidate(uint8_t index, uint8_t value);
extern void APP_nvValueChanged(uint8_t index, uint8_t value, uint8_t oldValue);

/**
 * The factoryReset for the NV service. Requests the application for defaults
 * for each NV and write those values to the non-volatile memory (NVM) store.
 */
void nvFactoryReset(void) {
    uint8_t i;
    for (i=1; i<= NV_NUM; i++) {
        if (i==0) break;
        writeNVM(NV_NVM_TYPE, i, APP_nvDefault(i));
    }
}

/**
 * Upon power up read the NV values from NVM and fill the NV cache.
 */
void nvPowerUp(void) {
#ifdef NV_CACHE
    int temp;
    // initialise the cache
    uint8_t i;
    for (i=0; i<= NV_NUM; i++) {
        temp = readNVM(NV_NVM_TYPE, NV_ADDRESS+i);
        if (temp < 0) {
            // unsure how to handle an error here
        } else {
            nvCache[i] = (uint8_t)temp;
        }
    }
#endif
}


#ifdef NV_CACHE
// TODO load the NV cache
#endif

/**
 * Get the value of an NV. Either obtained from the cache or from NVM.
 * @param index the NV index
 * @return the NV value
 */
int16_t getNV(uint8_t index) {
    if (index == 0) return NV_NUM;
    if (index > NV_NUM) return -CMDERR_INV_NV_IDX;
#ifdef NV_CACHE
    return nvCache[index];
#else
    return readNVM(NV_NVM_TYPE, NV_ADDRESS+index);
#endif
}

/**
 * Set (write) the value of an NV.
 * The module's application is called (APP_nvValidate) to check that the 
 * value being written is valid.
 * After updating the NV the application's function APP_nvValueChanged is
 * called so that the application can perform any related checks, changes or 
 * functionality.
 * 
 * @param index the NV index
 * @param value the value of the NV to be written 
 * @return 0 for no error otherwise the error
 */
uint8_t setNV(uint8_t index, uint8_t value) {
    uint8_t check;
    uint8_t oldValue;
    
    if (index > NV_NUM) return CMDERR_INV_NV_IDX;
    check = APP_nvValidate(index, value);
    if (check) return CMDERR_INV_NV_VALUE;
#ifdef NV_CACHE
    oldValue = nvCache[index];
    nvCache[index] = value;
    check = 0;
#else
    oldValue = readNVM(NV_NVM_TYPE, NV_ADDRESS+index);
    check = writeNVM(NV_NVM_TYPE, NV_ADDRESS+index, value);
#endif
    APP_nvValueChanged(index, value, oldValue);
    return check;
}

// shortcuts
#define NV_NNHI     bytes[0]
#define NV_NNLO     bytes[1]
#define NV_INDEX    bytes[2]
#define NV_VALUE    bytes[3]

/**
 * Process the NV related messages.
 * @param m the MERGLCB message
 * @return 1 if the message was processed, 0 otherwise
 */
uint8_t nvProcessMessage(Message * m) {
    int16_t valueOrError;
    // check NN matches us
    if (m->NV_NNHI != nn.bytes.hi) return 0;
    if (m->NV_NNLO != nn.bytes.lo) return 0;
    
    switch (m->opc) {
        case OPC_NVRD:
            if (m->len <= 3) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_CMD);
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, CMDERR_INV_CMD);
                return 1;
            }
            valueOrError = getNV(m->NV_INDEX);
            if (valueOrError < 0) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-valueOrError));
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, (uint8_t)(-valueOrError));
                return 1;
            }
            sendMessage3(OPC_NVANS, nn.bytes.hi, nn.bytes.lo, (uint8_t)(valueOrError));
            return 1;
        case OPC_NVSET:
            if (m->len <= 4) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_CMD);
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, CMDERR_INV_CMD);
                return 1;
            }
            valueOrError = setNV(m->NV_INDEX, m->NV_VALUE);
            if (valueOrError >0) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-valueOrError));
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, (uint8_t)(-valueOrError));
                return 1;
            }
            sendMessage2(OPC_WRACK, nn.bytes.hi, nn.bytes.lo);
            sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, GRSP_OK);
            return 1;
        case OPC_NVSETRD:
            if (m->len <= 4) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_CMD);
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, CMDERR_INV_CMD);
                return 1;
            }
            valueOrError = setNV(m->NV_INDEX, m->NV_VALUE);
            if (valueOrError >0) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-valueOrError));
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, (uint8_t)(-valueOrError));
                return 1;
            }
            valueOrError = getNV(m->NV_INDEX);
            if (valueOrError < 0) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-valueOrError));
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, (uint8_t)(-valueOrError));
                return 1;
            }
            sendMessage3(OPC_NVANS, nn.bytes.hi, nn.bytes.lo, (uint8_t)(valueOrError));
            return 1;
        default:
            return 0;   // message not processed
    }
}

        