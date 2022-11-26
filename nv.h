#include "merglcb.h"

extern Service nvService;

uint8_t APP_nvDefault(uint8_t index);

void nvFactoryReset(void);
void nvPowerUp(void);
void nvPoll(void);
uint8_t nvProcessMessage(Message * m);
