#include "merglcb.h"

extern Service canService;

void canFactoryReset(void);
void canPowerUp(void);
void canPoll(void);
unsigned char canProcessMessage(Message * m);
void canIsr(void);
