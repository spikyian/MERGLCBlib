#include "merglcb.h"

extern Service canService;

void canFactoryReset(void);
void canPowerUp(void);
void canPoll(void);
uint8_t canProcessMessage(Message * m);
void canIsr(void);
