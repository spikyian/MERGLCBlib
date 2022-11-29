#ifndef _TICKTIME_H_
/********************************************************************
* FileName:		TickTime.h
* Dependencies: TickTime.h
* Processor:	PIC18, PIC24F, PIC32, dsPIC30, dsPIC33
*               tested with 18F4620, dsPIC33FJ256GP710	
* Hardware:		PICDEM Z, Explorer 16, PIC18 Explorer
* Complier:     Microchip C18 v3.04 or higher
*				Microchip C30 v2.03 or higher
*               Microchip C32 v1.02 or higher	
* Company:		Microchip Technology, Inc.
*
* Copyright and Disclaimer Notice
*
* This module has been derived from the Microchip MLA Symboltime module,
* therefore the Microchip licensing terms apply.
*
* Copyright � 2007-2010 Microchip Technology Inc.  All rights reserved.
* Copyright � 2015 Pete Brownlow for changes Jan 2015
*
* Microchip licenses to you the right to use, modify, copy and distribute 
* Software only when embedded on a Microchip microcontroller or digital 
* signal controller, which are integrated into your product or third party
* product (pursuant to the terms in the accompanying license agreement).   
*
* You should refer to the license agreement accompanying this Software for 
* additional information regarding your rights and obligations.
*
* SOFTWARE AND DOCUMENTATION ARE PROVIDED �AS IS� WITHOUT WARRANTY OF ANY 
* KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY 
* WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A 
* PARTICULAR PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE 
* LIABLE OR OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, 
* CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY 
* DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO 
* ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, 
* LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, 
* TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT 
* NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
*
*********************************************************************
* File Description:
*
*  This file provides access to all of the time management functions
*   as well as calculating the timer scaling settings required for
*   accurate symbol time measurement
*
* Change History:
*  Rev   Date         Author    Description
*  0.1   11/09/2006   yfy       Initial revision
*  1.0   01/09/2007   yfy       Initial release
*  2.0   4/15/2009    yfy       MiMAC and MiApp revision
*  2.1   06/20/2009   yfy       Add LCD support
*  3.1   5/28/2010    yfy       MiWi DE 3.1
*  4.1   6/3/2011     yfy       MAL v2011-06
*  4.2   15/1/15      pnb       Extracted from MLA as standalone utility (C18 only at present)
*                               Timer prescaler calculated from clock MHz set in hwsettings
*  4.3   Nov 2022     ih        Port to XC8 and updated for needs of MERGLCB 
*
********************************************************************/
#define _TICKTIME_H_
/////////////////////////////////////////////////////
// TickTime
/////////////////////////////////////////////////////
#if defined(CPUF18K) || defined(_PIC18)
    /* this section is based on the Timer 0 module of the PIC18 family */

//   Prescaler is now calculated from clock MHz in the init routine - which assumes clock is an exact number of MHz
//    #define ONE_SECOND (((DWORD)CLOCK_FREQ/1000 * 62500) / (SYMBOL_TO_TICK_RATE / 1000))
//    /* SYMBOLS_TO_TICKS to only be used with input (a) as a constant, otherwise you will blow up the code */
//    #define SYMBOLS_TO_TICKS(a) (((DWORD)CLOCK_FREQ/100000) * a / ((DWORD)SYMBOL_TO_TICK_RATE/100000))
//    #define TICKS_TO_SYMBOLS(a) (((DWORD)SYMBOL_TO_TICK_RATE/100000) * a / ((DWORD)CLOCK_FREQ/100000))


    #define TMR_IF          INTCONbits.TMR0IF
    #define TMR_IE          INTCONbits.TMR0IE
    #define TMR_IP          INTCON2bits.TMR0IP
    #define TMR_ON          T0CONbits.TMR0ON
    #define TMR_CON         T0CON
    #define TMR_L           TMR0L
    #define TMR_H           TMR0H
    
    
#elif defined(__dsPIC30F__) || defined(__dsPIC33F__) || defined(__PIC24F__) || defined(__PIC24FK__) || defined(__PIC24H__)
    /* this section is based on the Timer 2/3 module of the dsPIC33/PIC24 family */
    #if(CLOCK_FREQ <= 125000)
        #define CLOCK_DIVIDER 1
        #define CLOCK_DIVIDER_SETTING 0x0000 /* no prescalar */
        #define SYMBOL_TO_TICK_RATE 125000
    #elif(CLOCK_FREQ <= 1000000)
        #define CLOCK_DIVIDER 8
        #define CLOCK_DIVIDER_SETTING 0x0010
        #define SYMBOL_TO_TICK_RATE 1000000
    #elif(CLOCK_FREQ <= 8000000)
        #define CLOCK_DIVIDER 64
        #define CLOCK_DIVIDER_SETTING 0x0020
        #define SYMBOL_TO_TICK_RATE 8000000
    #else
        #define CLOCK_DIVIDER 256
        #define CLOCK_DIVIDER_SETTING 0x0030
        #define SYMBOL_TO_TICK_RATE 32000000
    #endif

    #define ONE_SECOND (((DWORD)CLOCK_FREQ/1000 * 62500) / ((DWORD)SYMBOL_TO_TICK_RATE / 1000))
    /* SYMBOLS_TO_TICKS to only be used with input (a) as a constant, otherwise you will blow up the code */
    #define SYMBOLS_TO_TICKS(a) (((DWORD)CLOCK_FREQ/10000 * a ) / ((DWORD)SYMBOL_TO_TICK_RATE / 10000))
    #define TICKS_TO_SYMBOLS(a) (((DWORD)SYMBOL_TO_TICK_RATE/10000) * a / ((DWORD)CLOCK_FREQ/10000))
#elif defined(__PIC32MX__)
    /* this section is based on the Timer 2/3 module of the PIC32MX family */
    #define INSTR_FREQ  (CLOCK_FREQ/4)
    #if(INSTR_FREQ <= 125000)
        #define CLOCK_DIVIDER 1
        #define CLOCK_DIVIDER_SETTING 0x0000 /* no prescalar */
        #define SYMBOL_TO_TICK_RATE 125000
    #elif(INSTR_FREQ <= 1000000)
        #define CLOCK_DIVIDER 8
        #define CLOCK_DIVIDER_SETTING 0x0030
        #define SYMBOL_TO_TICK_RATE 1000000
    #elif(INSTR_FREQ <= 8000000)
        #define CLOCK_DIVIDER 64
        #define CLOCK_DIVIDER_SETTING 0x0060
        #define SYMBOL_TO_TICK_RATE 8000000
    #elif(INSTR_FREQ <= 16000000)
        #define CLOCK_DIVIDER 256
        #define CLOCK_DIVIDER_SETTING 0x0070
        #define SYMBOL_TO_TICK_RATE INSTR_FREQ
    #else
        #define CLOCK_DIVIDER 256
        #define CLOCK_DIVIDER_SETTING 0x70
        #define SYMBOL_TO_TICK_RATE INSTR_FREQ
    #endif

    #define ONE_SECOND (((DWORD)INSTR_FREQ/1000 * 62500) / (SYMBOL_TO_TICK_RATE / 1000))
    /* SYMBOLS_TO_TICKS to only be used with input (a) as a constant, otherwise you will blow up the code */
    #define SYMBOLS_TO_TICKS(a) (((DWORD)(INSTR_FREQ/100000) * a) / (SYMBOL_TO_TICK_RATE / 100000))
    #define TICKS_TO_SYMBOLS(a) (((DWORD)SYMBOL_TO_TICK_RATE/100000) * a / ((DWORD)CLOCK_FREQ/100000))
#else
    //#error "Unsupported processor.  New timing definitions required for proper operation"
#endif

#define HUNDRED_MICRO_SECOND 6                      // 6 ticks is 96us - approx 100us as close as possible with 16uS resolution
#define ONE_SECOND          62500                   // Ticks per second at 16uS per tick
#define TWO_SECOND          (ONE_SECOND*2)
#define FIVE_SECOND         (ONE_SECOND*5)
#define TEN_SECOND          (ONE_SECOND*10)
#define HALF_SECOND         (ONE_SECOND/2)
#define HALF_MILLI_SECOND   (ONE_SECOND/2000)
#define ONE_MILI_SECOND     (ONE_SECOND/1000)
#define HUNDRED_MILI_SECOND (ONE_SECOND/10)
#define FORTY_MILI_SECOND   (ONE_SECOND/25)
#define TWENTY_MILI_SECOND  (ONE_SECOND/50)
#define TEN_MILI_SECOND     (ONE_SECOND/100)
#define FIVE_MILI_SECOND    (ONE_SECOND/200)
#define TWO_MILI_SECOND     (ONE_SECOND/500)
#define ONE_MINUTE          (ONE_SECOND*60)
#define ONE_HOUR            (ONE_MINUTE*60)

#define tickGetDiff(a,b) (a.val - b.val)
#define tickTimeSince(t)    (tickGet() - t.val)

/************************ DATA TYPES *******************************/


/******************************************************************
 // Time unit defined based on IEEE 802.15.4 specification.
 // One tick is equal to one symbol time, or 16us. The Tick structure
 // is four bytes in length and is capable of represent time up to
 // about 19 hours.
 *****************************************************************/
typedef union _TickValue
{
    uint32_t val;
    struct TickBytes
    {
        uint8_t b0;
        uint8_t b1;
        uint8_t b2;
        uint8_t b3;
    } byte;
    uint8_t v[4];
    struct TickWords
    {
        uint16_t w0;
        uint16_t w1;
    } word;
} TickValue;


// Global routine definitions

/**
 * Sets up Timer0 to count time.
 * @param priority 0=low priority, high priority otherwise
 */
void initTicker(uint8_t priority);
/**
 * Gets the current tick counter indicating time since power on.
 * @return the value of the timer
 */
uint32_t tickGet(void);
/**
 * The Timer interrupt service routine.
 */
void tickISR(void);

/************************ VARIABLES ********************************/
/**
 * Timer0 provides a 16bit counter. The timerExtension variables extend 
 * the count to 32bit.
 */
extern volatile uint8_t timerExtension1,timerExtension2;

#endif