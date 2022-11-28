#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#ifdef _PIC18

// How to determine whether interrupts are enabled
#define geti()  (INTCONbits.GIE)
#endif

#endif