#include "merglcb.h"

extern Service mnsService;

void mnsFactoryReset(void);
void mnsPowerUp(void);
void mnsPoll(void);
unsigned char mnsProcessMessage(Message * m);

extern Word nn;
extern unsigned char mode;
