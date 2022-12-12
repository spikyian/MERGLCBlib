/*
  This work is licensed under the:
      Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
   To view a copy of this license, visit:
      http://creativecommons.org/licenses/by-nc-sa/4.0/
   or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

   License summary:
    You are free to:
      Share, copy and redistribute the material in any medium or format
      Adapt, remix, transform, and build upon the material

    The licensor cannot revoke these freedoms as long as you follow the license terms.

    Attribution : You must give appropriate credit, provide a link to the license,
                   and indicate if changes were made. You may do so in any reasonable manner,
                   but not in any way that suggests the licensor endorses you or your use.

    NonCommercial : You may not use the material for commercial purposes. **(see note below)

    ShareAlike : If you remix, transform, or build upon the material, you must distribute
                  your contributions under the same license as the original.

    No additional restrictions : You may not apply legal terms or technological measures that
                                  legally restrict others from doing anything the license permits.

   ** For commercial use, please contact the original copyright holder(s) to agree licensing terms

    This software is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE

  Ian Hogg Nov 2022
 */

/*
 * This is the main() code.
 */
#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "romops.h"
#include "hardware.h"

/*
 * This are some hacks to ensure that the ISRs are located at addresses to
 * give space for the parameter block.
 */
#ifdef __18CXX
void ISRLow(void);
void ISRHigh(void);

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
uint8_t eeBootFlag = 0;
#endif

extern void setup(void);
extern void loop(void);

void main(void) {
    uint8_t i;
    
#if defined(_PIC18)
    RCONbits.IPEN = 1;  // enable interrupt priority
#endif
     
    // init the romops ready for flash writes
    initRomOps();
    
    if (readNVM(NV_NVM_TYPE, NV_ADDRESS) != APP_NVM_VERSION) {
        factoryReset();
    }
    
    // initialise the services
    powerUp();
    
    // call the application's init 
    setup();
    
    // enable the interrupts and ready to go
    bothEi();   
    while(1) {
        // poll the services as quickly as possible.
        // up to service to ignore the polls it doesn't need.
        poll();
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

 
// CONFIG1L
#pragma config RETEN = OFF      // VREG Sleep Enable bit (Ultra low-power regulator is Disabled (Controlled by REGSLP bit))
#pragma config INTOSCSEL = HIGH // LF-INTOSC Low-power Enable bit (LF-INTOSC in High-power mode during Sleep)
#pragma config SOSCSEL = DIG    // SOSC Power Selection and mode Configuration bits (Digital (SCLKI) mode)
#pragma config XINST = OFF      // Extended Instruction Set (Disabled)

// CONFIG1H
#pragma config FOSC = HS1       // Oscillator (HS oscillator (Medium power, 4 MHz - 16 MHz))
#pragma config PLLCFG = OFF      // PLL x4 Enable bit (Disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor (Disabled)
#pragma config IESO = OFF       // Internal External Oscillator Switch Over Mode (Disabled)

// CONFIG2L
#pragma config PWRTEN = ON      // Power Up Timer (Enabled)
#pragma config BOREN = SBORDIS      // Brown Out Detect (Disabled in hardware, SBOREN disabled)
#pragma config BORV = 0         // Brown-out Reset Voltage bits (3.0V)
#pragma config BORPWR = ZPBORMV // BORMV Power level (ZPBORMV instead of BORMV is selected)

// CONFIG2H
#pragma config WDTEN = OFF      // Watchdog Timer (WDT disabled in hardware; SWDTEN bit disabled)
#pragma config WDTPS = 1048576      // Watchdog Postscaler (1:1048576)

// CONFIG3H
#pragma config CANMX = PORTB    // ECAN Mux bit (ECAN TX and RX pins are located on RB2 and RB3, respectively)
#pragma config MSSPMSK = MSK7   // MSSP address masking (7 Bit address masking mode)
#pragma config MCLRE = ON       // Master Clear Enable (MCLR Enabled, RE3 Disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Overflow Reset (Enabled)
#pragma config BBSIZ = BB1K     // Boot Block Size (1K word Boot Block size)

// CONFIG5L
#pragma config CP0 = OFF        // Code Protect 00800-01FFF (Disabled)
#pragma config CP1 = OFF        // Code Protect 02000-03FFF (Disabled)
#pragma config CP2 = OFF        // Code Protect 04000-05FFF (Disabled)
#pragma config CP3 = OFF        // Code Protect 06000-07FFF (Disabled)

// CONFIG5H
#pragma config CPB = OFF        // Code Protect Boot (Disabled)
#pragma config CPD = OFF        // Data EE Read Protect (Disabled)

// CONFIG6L
#pragma config WRT0 = OFF       // Table Write Protect 00800-01FFF (Disabled)
#pragma config WRT1 = OFF       // Table Write Protect 02000-03FFF (Disabled)
#pragma config WRT2 = OFF       // Table Write Protect 04000-05FFF (Disabled)
#pragma config WRT3 = OFF       // Table Write Protect 06000-07FFF (Disabled)

// CONFIG6H
#pragma config WRTC = OFF       // Config. Write Protect (Disabled)
#pragma config WRTB = OFF       // Table Write Protect Boot (Disabled)
#pragma config WRTD = OFF       // Data EE Write Protect (Disabled)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protect 00800-01FFF (Disabled)
#pragma config EBTR1 = OFF      // Table Read Protect 02000-03FFF (Disabled)
#pragma config EBTR2 = OFF      // Table Read Protect 04000-05FFF (Disabled)
#pragma config EBTR3 = OFF      // Table Read Protect 06000-07FFF (Disabled)

// CONFIG7H
#pragma config EBTRB = OFF      // Table Read Protect Boot (Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.


