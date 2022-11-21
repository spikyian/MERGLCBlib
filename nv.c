#include <xc.h>
#include "merglcb.h"
#include "module.h"
// service definition
Service nvService = {SERVICE_ID_NV, 1, APP_factoryResetNv, powerUpNv, processMessageNv, pollNv, NULL};

// nv cache
#ifdef NV_CACHE
static unsigned char nvCache[NV_NUM+1];
#endif

void powerUpNv() {
#ifdef NV_CACHE
    // initialise the cache
    unsigned char i;
    for (i=0; i<= NV_NUM; i++) {
        nvCache[i] = readNVM(NV_NVM_TYPE, NV_ADDRESS+i);
    }
#endif
}

// poll
void pollNv() {
    
}

// No NV ISR

// diagnostics

int getNV(unsigned char index) {
    if (index == 0) return NV_NUM;
    if (index > NV_NUM) return -CMDERR_INV_NV_IDX;
#ifdef NV_CACHE
    return nvCache[index];
#else
    return readNVM(NV_NVM_TYPE, NV_ADDRESS+index);
#endif
}

unsigned char setNV(unsigned char index, unsigned char value) {
    unsigned char check;
    unsigned char oldValue;
    
    if (index > NV_NUM) return CMDERR_INV_NV_IDX;
    check = APP_nvValidate(index, value);
    if (check) return CMDERR_INV_NV_VALUE;
#ifdef NV_CACHE
    oldValue = nvCache[index];
    nvCache[index] = value;
    APP_nvValueChanged(index, value, oldValue);
    return 0;
#else
    oldValue = readNVM(NV_NVM_TYPE, NV_ADDRESS+index);
    check = writeNVM(NV_NVM_TYPE, NV_ADDRESS+index, value);
    APP_nvValueChanged(index, value, oldValue);
#endif
    return check;
}

#define NV_NNHI     bytes[0]
#define NV_NNLO     bytes[1]
#define NV_INDEX    bytes[2]
#define NV_VALUE    bytes[3]


unsigned char processMessageNv(Message * m) {
    unsigned char valueOrError;
    // check NN matches us
    if (m->NV_NNHI != NN.hi) return 0;
    if (m->NV_NNLO != NN.lo) return 0;
    
    switch (m->opc) {
        case OPC_NVRD:
            if (m->len <= 3) {
                sendMessage(OPC_CMDERR, NN.hi, NN.lo, CMDERR_INV_CMD);
                sendMessage(OPC_GRSP, NN.hi, NN.lo, OPC_NVRD, SERVICE_ID_MNS, CMDERR_INV_CMD);
                return 1;
            }
            valueOrError = getNV(m->NV_INDEX);
            if (valueOrError < 0) {
                sendMessage(OPC_CMDERR, NN.hi, NN.lo, (unsigned char)(-valueOrError));
                sendMessage(OPC_GRSP, NN.hi, NN.lo, OPC_NVRD, SERVICE_ID_MNS, (unsigned char)(-valueOrError));
                return 1;
            }
            sendMessage(OPC_NVANS, NN.hi, NN.lo, (unsigned char)(valueOrError));
            return 1;
        case OPC_NVSET:
            if (m->len <= 4) {
                sendMessage(OPC_CMDERR, NN.hi, NN.lo, CMDERR_INV_CMD);
                sendMessage(OPC_GRSP, NN.hi, NN.lo, OPC_NVRD, SERVICE_ID_MNS, CMDERR_INV_CMD);
                return 1;
            }
            valueOrError = setNV(m->NV_INDEX, m->NV_VALUE);
            if (valueOrError >0) {
                sendMessage(OPC_CMDERR, NN.hi, NN.lo, valueOrError);
                sendMessage(OPC_GRSP, NN.hi, NN.lo, OPC_NVRD, SERVICE_ID_MNS, valueOrError);
                return 1;
            }
            sendMessage(OPC_WRACK, NN.hi, NN.lo);
            sendMessage(OPC_GRSP, NN.hi, NN.lo, OPC_NVRD, SERVICE_ID_MNS, GRSP_OK);
            return 1;
        case OPC_NVSETRD:
            if (m->len <= 4) {
                sendMessage(OPC_CMDERR, NN.hi, NN.lo, CMDERR_INV_CMD);
                sendMessage(OPC_GRSP, NN.hi, NN.lo, OPC_NVRD, SERVICE_ID_MNS, CMDERR_INV_CMD);
                return 1;
            }
            valueOrError = setNV(m->NV_INDEX, m->NV_VALUE);
            if (valueOrError >0) {
                sendMessage(OPC_CMDERR, NN.hi, NN.lo, valueOrError);
                sendMessage(OPC_GRSP, NN.hi, NN.lo, OPC_NVRD, SERVICE_ID_MNS, valueOrError);
                return 1;
            }
            valueOrError = getNV(m->NV_INDEX);
            if (valueOrError < 0) {
                sendMessage(OPC_CMDERR, NN.hi, NN.lo, (unsigned char)(-valueOrError));
                sendMessage(OPC_GRSP, NN.hi, NN.lo, OPC_NVRD, SERVICE_ID_MNS, (unsigned char)(-valueOrError));
                return 1;
            }
            sendMessage(OPC_NVANS, NN.hi, NN.lo, (unsigned char)(valueOrError));
            return 1;
        default:
            return 0;   // message not processed
    }
}

        