/**
 * @copyright Copyright � 2007-2010 Microchip Technology Inc.  All rights reserved.
 */
/*
* FileName:		TickTime.c
* Dependencies: TickTime.h
* Processor:	PIC18, PIC24, PIC32, dsPIC30, dsPIC33
*               tested with 18F4620, dsPIC33FJ256GP710
* Complier:     Microchip C18 v3.04 or higher
*				Microchip C30 v2.03 or higher	
*               Microchip C32 v1.02 or higher
* Company:	Microchip Technology, Inc.
*
* Copyright and Disclaimer Notice
*
* This module has been derived from the Microchip MLA Symboltime module,
* therefore the Microchip licensing terms apply.
*
* Copyright � 2007-2010 Microchip Technology Inc.  All rights reserved.
* Copyright � 2015 Pete Brownlow for changes Jan 2015
* Ported to XC8 by Ian Hogg 23/5/2017
*
* Microchip licenses to you the right to use, modify, copy and distribute 
* Software only when embedded on a Microchip microcontroller or digital 
* signal controller and used with a Microchip radio frequency transceiver, 
* which are integrated into your product or third party product (pursuant 
* to the terms in the accompanying license agreement).  
*
* You should refer to the license agreement accompanying this Software for 
* additional information regarding your rights and obligations.
*
* SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” WITHOUT WARRANTY OF ANY 
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
*   This file implements functions used for smybol timer.
*
* Change History:
*  Rev   Date         Author    Description
*  0.1   11/09/2006   yfy       Initial revision
*  1.0   01/09/2007   yfy       Initial release
*  2.0   4/15/2009    yfy       MiMAC and MiApp revision
*  2.1   6/20/2009    yfy       Add LCD support
*  3.1   5/28/2010    yfy       MiWi DE 3.1
*  4.1   6/3/2011     yfy       MAL v2011-06
*  4.2   15/1/15      pnb       Extracted from MLA as a standalone utility (C18 only at present)
 * 4.3   Nov 2022     ih        Ported to XC8 and modified for use by MERGLCB
********************************************************************/
/**
 * @author Copyright � 2007-2010 Microchip Technology Inc.  All rights reserved.
 * @author Copyright � 2015 Pete Brownlow for changes Jan 2015
 * @author Ported to XC8 by Ian Hogg 23/5/2017
 * @author Updated for MERGLCB Ian Hogg Nov 2022
 * @date Nov 2022
 */

/**
 * @file
 * Functions to provide timing.
 * Uses 16bit PIC Timer0. Extends this to 32bit using timerExtension which is 
 * incremented on timer0 overflow.
 * Times are stored as TickValue.
 * Functions are provided to measure time since a time value was recorded.
 * 
 */

#include <xc.h>
#include "ticktime.h"
#include "module.h"

volatile uint8_t timerExtension1,timerExtension2;

/************************ FUNCTIONS ********************************/

/*********************************************************************
* Function:         void InitTicker()
*
* PreCondition:     none
*
* Input:		    none
*
* Output:		    none
*
* Side Effects:	    TMR0 for PIC18 is configured for calculating
*                   the correct symbol times.  TMR2/3 for PIC24/dsPIC
*                   is configured for calculating the correct symbol
*                   times
*
* Overview:		    This function will configure the UART for use at 
*                   in 8 bits, 1 stop, no flowcontrol mode
*
* Note:		    The timer interrupt is enabled causing the timer
*                   roll over calculations.  Interrupts are required
*                   to be enabled in order to extend the timer to
*                   4 bytes in PIC18.  PIC24/dsPIC version do not 
*                   enable or require interrupts
********************************************************************/

void initTicker(uint8_t priority) {
    uint8_t divider, i;

    divider = 0;
    for (i=clkMHz;i>0;i>>=1) // Work out timer prescaler value from clock MHz
        divider++;

#if defined(__18CXX) || defined (__XC8)
    TMR_CON = (uint8_t)(0b00000000 | divider);     // Enable internal clock, prescaler on and set prescaler value
    TMR_H = 0;          // clear the H buffer
    TMR_L = 0;          // write the L counter and load the H counter from buffer
    TMR_IP = priority;  // set interrupt priority
    TMR_IF = 0;         // clear the flag
    TMR_IE = 1;         // enable interrupts
    TMR_ON = 1;         // start it running

    timerExtension1 = 0;
    timerExtension2 = 0;
#elif defined(__dsPIC30F__) || defined(__dsPIC33F__) || defined(__PIC24F__) || defined(__PIC24FK__) || defined(__PIC24H__)
    T2CON = 0b0000000000001000 | CLOCK_DIVIDER_SETTING;
    T2CONbits.TON = 1;
#elif defined(__PIC32MX__)
    CloseTimer2();
    WriteTimer2(0x00);
    WriteTimer3(0x00);
    WritePeriod3(0xFFFF);
    OpenTimer2((T2_ON|T2_32BIT_MODE_ON|CLOCK_DIVIDER_SETTING),0xFFFFFFFF);     
#else
    #error " timer implementation required for stack usage."
#endif
}


/*********************************************************************
* Function:         void tickGet()
*
* PreCondition:     none
*
* Input:		    none
*
* Output:		    TickValue - the current tick time
*
* Side Effects:	    PIC18 only: the timer interrupt is disabled
*                   for several instruction cycles while the 
*                   timer value is grabbed.  This is to prevent a
*                   rollover from incrementing the timer extenders
*                   during the read of their values
*
* Overview:		    This function returns the current time
*
* Note:			    PIC18 only:
*                   caution: this function should never be called 
*                   when interrupts are disabled if interrupts are 
*                   disabled when this is called then the timer 
*                   might rollover and the byte extension would not 
*                   get updated.
********************************************************************/
uint32_t tickGet(void) {
    TickValue currentTime;
    
#if defined(__18CXX) || defined(__XC8)
    //uint8_t failureCounter;
    uint8_t IntFlag1;
    uint8_t IntFlag2;
    
    /* zero the byte extension for now*/
    currentTime.byte.b2 = 0;
    currentTime.byte.b3 = 0;
    /* disable the timer interrupt to prevent roll over of the lower 16 bits while before/after reading of the extension */
    TMR_IE = 0;
#if 1
    do
    {
        IntFlag1 = TMR_IF;
        currentTime.byte.b0 = TMR_L;
        currentTime.byte.b1 = TMR_H;    // PIC latched the H register whist reading the L register. Safe 2 byte read.
        IntFlag2 = TMR_IF;
    } while(IntFlag1 != IntFlag2);  // verify that a rollover didn't happen during getting the counter

    if( IntFlag1 > 0 )          // if a rollover did happen then handle it here instead of in ISR
    {
        TMR_IF = 0;
        timerExtension1++;
        if(timerExtension1 == 0)
        {
            timerExtension2++;
        }
    }
#else

    
    failureCounter = 0;
    /* read the timer value */
    do
    {
        currentTime.byte.b0 = TMR_L;
        currentTime.byte.b1 = TMR_H;
    } while( currentTime.word.w0 == 0xFFFF && failureCounter++ < 3 );

    //if an interrupt occurred after IE = 0, then we need to figure out if it was
    //before or after we read TMR0L
    if(TMR_IF)
    {
        //if(currentTime.byte.b0<10)
        {
            //if we read TMR0L after the rollover that caused the interrupt flag then we need
            //to increment the 3rd byte
            currentTime.byte.b2++;  //increment the upper most
            if(timerExtension1 == 0xFF)
            {
                currentTime.byte.b3++;
            }
        }
    }
#endif

    /* copy the byte extension */
    currentTime.byte.b2 += timerExtension1;
    currentTime.byte.b3 += timerExtension2;
    
    /* re-enable the timer interrupt */
    TMR_IE = 1;
    
#elif defined(__dsPIC30F__) || defined(__dsPIC33F__) || defined(__PIC24F__) || defined(__PIC24FK__) || defined(__PIC24H__) || defined(__PIC32MX__)
    currentTime.word.w1 = TMR3;
    currentTime.word.w0 = TMR2;
    if( currentTime.word.w1 != TMR3 )
   {
       currentTime.word.w1 = TMR3;
       currentTime.word.w0 = TMR2;
    }
#else
    #error "Symbol timer implementation required for stack usage."
#endif
    return currentTime.val;
} // tickGet

