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
#include "timedResponse.h"

// forward declarations
static void loadNVcahe(void);
void nvFactoryReset(void);
void nvPowerUp(void);
Processed nvProcessMessage(Message *m);
TimedResponseResult nvTRnvrdCallback(uint8_t type, const Service * s, uint8_t step);

#define TIMED_RESPONSE_NVRD OPC_NVRD

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
extern void APP_nvValueChanged(uint8_t index, uint8_t value, uint8_t oldValue);

/**
 * The factoryReset for the NV service. Requests the application for defaults
 * for each NV and write those values to the non-volatile memory (NVM) store.
 */
void nvFactoryReset(void) {
    uint8_t i;
    for (i=1; i<= NV_NUM; i++) {
        writeNVM(NV_NVM_TYPE, NV_ADDRESS+i, APP_nvDefault(i));
    }
}

/**
 * Upon power up read the NV values from NVM and fill the NV cache.
 */
void nvPowerUp(void) {
#ifdef NV_CACHE
    loadNVcahe();
#endif
}


#ifdef NV_CACHE
/**
 * Load the NV cache using values stored in non volatile memory.
 */
static void loadNVcahe(void) {
    uint8_t i;
    int16_t temp;
    
    for (i=1; i<= NV_NUM; i++) {
        temp = readNVM(NV_NVM_TYPE, NV_ADDRESS+i);
        if (temp < 0) {
            // unsure how to handle an error here
        } else {
            nvCache[i] = (uint8_t)temp;
        }
    }
}
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
    uint8_t oldValue;
    
    if (index > NV_NUM) return CMDERR_INV_NV_IDX;
    if (APP_nvValidate(index, value) == INVALID) return CMDERR_INV_NV_VALUE;
#ifdef NV_CACHE
    oldValue = nvCache[index];
    nvCache[index] = value;
    writeNVM(NV_NVM_TYPE, NV_ADDRESS+index, value);
#else
    oldValue = readNVM(NV_NVM_TYPE, NV_ADDRESS+index);
    writeNVM(NV_NVM_TYPE, NV_ADDRESS+index, value);
#endif
    APP_nvValueChanged(index, value, oldValue);
    return 0;
}

/**
 * Process the NV related messages.
 * @param m the MERGLCB message
 * @return PROCESSED if the message was processed, NOT_PROCESSED otherwise
 */
Processed nvProcessMessage(Message * m) {
    int16_t valueOrError;
    
    if (m->len < 3) {
        return NOT_PROCESSED;
    }
    // check NN matches us
    if (m->bytes[0] != nn.bytes.hi) return NOT_PROCESSED;
    if (m->bytes[1] != nn.bytes.lo) return NOT_PROCESSED;
    
    switch (m->opc) {
        case OPC_NVRD:
            if (m->len < 4) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_CMD);
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, CMDERR_INV_CMD);
                return PROCESSED;
            }
            valueOrError = getNV(m->bytes[2]);
            if (valueOrError < 0) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-valueOrError));
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, (uint8_t)(-valueOrError));
                return PROCESSED;
            }
            sendMessage4(OPC_NVANS, nn.bytes.hi, nn.bytes.lo, m->bytes[2], (uint8_t)(valueOrError));
            if (m->bytes[2] == 0) {
                // a NVANS response for all of the NVs
                startTimedResponse(TIMED_RESPONSE_NVRD, SERVICE_ID_NV, &nvTRnvrdCallback);
            }
            return PROCESSED;
        case OPC_NVSET:
            if (m->len < 5) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_CMD);
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, CMDERR_INV_CMD);
                return PROCESSED;
            }
            valueOrError = setNV(m->bytes[2], m->bytes[3]);
            if (valueOrError >0) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-valueOrError));
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, (uint8_t)(-valueOrError));
                return PROCESSED;
            }
            sendMessage2(OPC_WRACK, nn.bytes.hi, nn.bytes.lo);
            sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, GRSP_OK);
            return PROCESSED;
        case OPC_NVSETRD:
            if (m->len < 5) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_CMD);
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, CMDERR_INV_CMD);
                return PROCESSED;
            }
            valueOrError = setNV(m->bytes[2], m->bytes[3]);
            if (valueOrError >0) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-valueOrError));
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, (uint8_t)(-valueOrError));
                return PROCESSED;
            }
            valueOrError = getNV(m->bytes[2]);
            if (valueOrError < 0) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, (uint8_t)(-valueOrError));
                sendMessage5(OPC_GRSP, nn.bytes.hi, nn.bytes.lo, OPC_NVRD, SERVICE_ID_MNS, (uint8_t)(-valueOrError));
                return PROCESSED;
            }
            sendMessage3(OPC_NVANS, nn.bytes.hi, nn.bytes.lo, (uint8_t)(valueOrError));
            return PROCESSED;
        default:
            return NOT_PROCESSED;   // message not processed
    }
}

/**
 * This is the callback used by the service discovery responses.
 * @param type always set to TIMED_RESPONSE_NVRD
 * @param s indicates the service requesting the responses
 * @param step loops through each service to be discovered
 * @return whether all of the responses have been sent yet.
 */
TimedResponseResult nvTRnvrdCallback(uint8_t type, const Service * s, uint8_t step) {
    int16_t valueOrError;
    if (step >= NV_NUM) {
        return TIMED_RESPONSE_RESULT_FINISHED;
    }
    valueOrError = getNV(step);
    if (valueOrError < 0) {
        return TIMED_RESPONSE_RESULT_FINISHED;
    }
    sendMessage3(OPC_NVANS, nn.bytes.hi, nn.bytes.lo, (uint8_t)(valueOrError));
    return TIMED_RESPONSE_RESULT_NEXT;
}