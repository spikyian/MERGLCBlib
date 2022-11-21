#include <xc.h>

#include <module.h>
#include "service.h"

Service * services[NUMSERVICES];

Service * findService(unsigned int id) {
    unsigned char i;
    for (i=0; i<NUMSERVICES; i++) {
        if (services[i]->serviceNo == id) return services[i];
    }
    return NULL;
}

unsigned char have(unsigned char id) {
    unsigned char i;
    for (i=0; i<NUMSERVICES; i++) {
        if (services[i]->serviceNo == id) return 1;
    }
    return 0;
}