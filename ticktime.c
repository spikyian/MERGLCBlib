///////////////////////////////////////////////////////////
// TICKTIME
///////////////////////////////////////////////////////////
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

void initTicker(uint8_t priority)
{
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
uint32_t tickGet(void)
{
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

