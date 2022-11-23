#include <xc.h>
#include "merglcb.h"
#include "module.h"

#ifdef __18CXX
void ISRLow(void);
void ISRHigh(void);
#endif

#ifdef __18CXX
void high_irq_errata_fix(void);

/*
 * Interrupt vectors (moved higher when bootloader present)
 */

// High priority interrupt vector

#ifdef BOOTLOADER_PRESENT
    #pragma code high_vector=0x808
#else
    #pragma code high_vector=0x08
#endif


//void interrupt_at_high_vector(void)

void HIGH_INT_VECT(void)
{
    _asm
        CALL high_irq_errata_fix, 1
    _endasm
}

/*
 * See 18F2480 errata
 */
void high_irq_errata_fix(void) {
    _asm
        POP
        GOTO ISRHigh
    _endasm
}

// low priority interrupt vector

#ifdef BOOTLOADER_PRESENT
    #pragma code low_vector=0x818
#else
    #pragma code low_vector=0x18
#endif

void LOW_INT_VECT(void)
{
    _asm GOTO ISRLow _endasm
}
#endif

#ifdef BOOTLOADER_PRESENT
// ensure that the bootflag is zeroed
#pragma romdata BOOTFLAG
unsigned char eeBootFlag = 0;
#endif

extern void init(void);
extern void loop(void);

void main(void) {
    unsigned char i;
    
    // ensure that the services array is initialised to empty
    for (i=0; i<NUM_SERVICES; i++) {
        services[i] = NULL;
    }
    // call the application's init to add the services it needs
    init();
    
    if (readNVM(NV_NVM_TYPE, NV_ADDRESS) != APP_NVM_VERSION) {
        factoryReset();
    }
    // now initialise the services
    powerUp();
       
    while(1) {
        loop();
    }
}




// Interrupt service routines 
#if defined(_18F26K80)
void __interrupt(high_priority) __section("mainSec") isrHigh() {
#endif
#if defined(_18F26K83)
void __interrupt(high_priority, base(0x0808)) isrHigh() {
#endif

    highIsr();
}

/**
 * Low priority interrupt service handler.
 */
#if defined(_18F26K80)
void __interrupt(low_priority) __section("mainSec") isrLow() {
#endif
#if defined(_18F26K83)
void __interrupt(low_priority, base(0x0808)) isrLow() {
#endif

    lowIsr();
}

 

