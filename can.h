#include "merglcb.h"

extern const Service canService;

void canFactoryReset(void);
void canPowerUp(void);
void canPoll(void);
uint8_t canProcessMessage(Message * m);
void canIsr(void);
extern DiagnosticVal * canGetDiagnostic(uint8_t index);
