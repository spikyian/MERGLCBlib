#include "merglcb.h"

typedef struct Service {
    unsigned char serviceNo;
    unsigned char version;
    void (* factoryReset)(void);
    void (* powerUp)(void);
    unsigned char (* processMessage)(Message * m);
    void (* poll)();
    void (* isr)();
    //void modes();
    //void statusCodes();
    //void diagnostics();
} Service;


