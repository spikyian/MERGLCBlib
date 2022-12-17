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
**************************************************************************************************************
	
 Ian Hogg Dec 2022
	
*/ 

#include <xc.h>
#include "merglcb.h"
#include "module.h"
#include "mns.h"

#include "arduino.h"

/* 
 * File:   util.c
 * Author: Ian
 *
 * Created on 8 December 2022
 * 
 * A collection of Arduino emulation functions
 * 
 */


#ifdef _PIC18
// PIN configs
const Config configs[] = {
    //PIN, PORT, PORT#, AN#
    {11, 'C', 0, 0xFF},   //0
    {12, 'C', 1, 0xFF},   //1
    {13, 'C', 2, 0xFF},   //2
    {14, 'C', 3, 0xFF},   //3
    {15, 'C', 4, 0xFF},   //4
    {16, 'C', 5, 0xFF},   //5
    {17, 'C', 6, 0xFF},   //6
    {18, 'C', 7, 0xFF},   //7
    {21, 'B', 0, 10},     //8
    {22, 'B', 1, 8},      //9
    {25, 'B', 4, 9},      //10
    {26, 'B', 5, 0xFF},   //11
    {3,  'A', 1, 1},      //12
    {2,  'A', 0, 0},      //13
    {5,  'A', 3, 3},      //14
    {7,  'A', 5, 4}       //15
};
#endif

void pinMode(uint8_t pin, PinMode mode) {
    if (pin < sizeof(configs)/sizeof(Config)) {
        // set digital/analogue first
        switch (mode) {
            case INPUT:
            case OUTPUT:
                if (configs[pin].an < 8) {
                    ANCON0 &= ~(1 << configs[pin].an);
                } else if (configs[pin].an < 16) {
                    ANCON1 &= ~(1 << (configs[pin].an - 8));
                }
                break;
            case ANALOGUE:
                if (configs[pin].an < 8) {
                    ANCON0 |= (1 << configs[pin].an);
                } else if (configs[pin].an < 16) {
                    ANCON1 |= (1 << (configs[pin].an - 8));
                }
                break;
        }
        // now set the digital port direction
        switch(configs[pin].port) {
            case 'A':
                switch (mode) {
                    case OUTPUT:
                        TRISA &= ~(1 << configs[pin].no);
                        break;
                    case INPUT:
                        TRISA |= (1 << configs[pin].no);
                        break;
                    case ANALOGUE:
                        break;
                }
                break;
            case 'B':
                switch (mode) {
                    case OUTPUT:
                        TRISB &= ~(1 << configs[pin].no);
                        break;
                    case INPUT:
                        TRISB |= (1 << configs[pin].no);
                        break;
                    case ANALOGUE:
                        break;
                }
                break;
            case 'C':
                switch (mode) {
                    case OUTPUT:
                        TRISC &= ~(1 << configs[pin].no);
                        break;
                    case INPUT:
                        TRISC |= (1 << configs[pin].no);
                        break;
                    case ANALOGUE:
                        break;
                }
                break;
            default:
                break;
        }
    }
}


void digitalWrite(uint8_t pin, uint8_t value) {
    if (pin < sizeof(configs)/sizeof(Config)) {
        // now set the digital port value
        switch(configs[pin].port) {
            case 'A':
                if (value) {
                    LATA |= (1 << configs[pin].no);
                } else {
                    LATA &= ~(1 << configs[pin].no);
                }
                break;
            case 'B':
                if (value) {
                    LATB |= (1 << configs[pin].no);
                } else {
                    LATB &= ~(1 << configs[pin].no);
                }
                break;
            case 'C':
                if (value) {
                    LATC |= (1 << configs[pin].no);
                } else {
                    LATC &= ~(1 << configs[pin].no);
                }
                break;
            default:
                break;
        }
    }
}

uint8_t digitalRead(uint8_t pin) {
    if (pin < sizeof(configs)/sizeof(Config)) {
        // now get the digital port value
        switch(configs[pin].port) {
            case 'A':
                return PORTA & (1 << configs[pin].no);
            case 'B':
                return PORTB & (1 << configs[pin].no);
            case 'C':
                return PORTC & (1 << configs[pin].no);
            default:
                break;
        }
    }
    return 0;
}