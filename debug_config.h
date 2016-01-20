/*
 * File: debug_config.h
 */

// copy the following defines if you want to use the LOG_LED  
//#define DEBUG_USE_LEDS

// also copy the following defines if you use the BREAK_LED
//#define DEBUG_USE_BUTTON

// the following define is required to use the serial breakpoint features
//#define DEBUG_SHARE_SERIAL
#define DEBUG_BAUDRATE      115200L // might need to be reduced if clock too slow

#define DEBUG_STR_SIZE  16          // >8
#define DEBUG_RX_PIN    0x11        // RC1
#define NUMBER_OF_PIN   24          // on PIC16F18855

// defines the user button on XPress
#define _BUTTON         PORTAbits.RA5
#define _BUTTON_MASK    0x20
#define _BUTTON_TRIS    TRISA
#define _BUTTON_ANSEL   ANSELA

// defines the port and position of LEDS on Xpress
#define _LED_MASK      0x0f
#define _LED_LAT       LATA
#define _LED_TRIS      TRISA
