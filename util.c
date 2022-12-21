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

#include "util.h"

/* 
 * File:   util.c
 * Author: Ian
 *
 * Created on 8 December 2022
 * 
 * A collection of useful utility functions
 * 
 */



/**
 * Checks that the required number of message bytes are present.
 * @param m the message to be checked
 * @param needed the number of bytes within the message needed
 * @return PROCESSED if it is an invalid message and should not be processed further
 */
Processed checkLen(Message * m, uint8_t needed) {
    if (m->len < needed) {
        if (m->len > 2) {
            if ((m->bytes[0] == nn.bytes.hi) && (m->bytes[1] == nn.bytes.lo)) {
                sendMessage3(OPC_CMDERR, nn.bytes.hi, nn.bytes.lo, CMDERR_INV_CMD);
            }
        }
        return PROCESSED;
    }
    return NOT_PROCESSED;
}

