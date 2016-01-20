/* 
 * File:   debug.h
 * Author: Lucio Di Jasio
 */

#ifndef DEBUG_H
#define	DEBUG_H

#include "debug_config.h"

#include "mcc_generated_files/mcc.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// simply display a nibble on the Xpress LEDs
#define LOG_LED(x)     do{ _LED_LAT &= ~_LED_MASK; _LED_LAT |= ((x) & _LED_MASK);} while(0)

// display a nibble on the XPress LEDs and wait for the user to press a button
#define BREAK_LED(x)   do{ LOG_LED(x); while(_BUTTON==1); while(_BUTTON==1); } while(0) 

void DEBUG_Initialize(void);

/** 
 * send a char to the debug serial port, 
 *  ex: LOG_CHAR('*');
 *  ->  *
 */
void LOG_CHAR(char x);

/** 
 * send a value y in ascii (hex)) to the debug serial port, 
 *  ex: WATCH_BYTE('2', bcount);
 *  ->  2:FF
*/
void WATCH_BYTE( char x, uint8_t y);

/** 
 * send a value y in ascii (hex)) to the debug serial port, 
 *  ex: WATCH_WORD('7', wcount);
 *  ->  7:12AB
*/
void WATCH_WORD( char x, uint8_t w);

/** 
 * send nth breakpoint and wait for the user to send back a char
 *  ex: BREAK_NUM(0x17);
 *  ->  17>    (expect char from terminal)
 */
void BREAK(uint8_t ); 

/**
 * send nth breakpoint, open a debugging console
 *  ex: DEBUG_BREAKPOINT(5);
 *  ->  5:      (expect debugging console commands or 'c' to continue)
 */
void BREAK_DBG(uint8_t brk);

// private functions
void _LOG_BYTE( uint8_t b);
char _DEBUG_Ack(void);



#endif	/* DEBUG_H */

