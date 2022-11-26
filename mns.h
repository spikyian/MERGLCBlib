#include "merglcb.h"

extern Service mnsService;

extern void mnsFactoryReset(void);
extern void mnsPowerUp(void);
extern void mnsPoll(void);
extern uint8_t mnsProcessMessage(Message * m);
extern void mnsLowIsr(void);

extern Word nn;
extern uint8_t mode;

typedef enum {
    OFF,            // fixed OFF
    ON,             // fixed ON
    FLASH_50_1HZ,   // 50% duty cycle 
    FLASH_50_HALF_HZ,   //
    SINGLE_FLICKER_OFF,
    SINGLE_FLICKER_ON
} LedState;

#define YELLOW_LED  0
#define GREEN_LED   1
