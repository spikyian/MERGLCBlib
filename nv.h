#include "merglcb.h"

extern Service nvService;

unsigned char APP_nvDefault(unsigned char index);

void nvFactoryReset(void);
void nvPowerUp(void);
void nvPoll(void);
unsigned char nvProcessMessage(Message * m);
