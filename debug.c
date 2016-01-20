/*
 * Debug Open API
 * 
 * Author: Lucio Di Jasio
 * 
 */
#include "debug.h"

#ifdef DEBUG_SHARE_SERIAL 
    #define __SERIAL_CAPTURE()  _DEBUG_SerialCapture()
    #define __SERIAL_RELEASE()  _DEBUG_SerialRelease()
#else
    #define __SERIAL_CAPTURE()  
    #define __SERIAL_RELEASE()  
#endif

/**
 * locals
 */
#ifdef DEBUG_SHARE_SERIAL
static uint8_t saved_tx_pin;
static uint8_t saved_rx_pin;
static uint8_t saved_baudrate;


#define IO_FUNCTION     0x00        // GPIO 
#define TX_FUNCTION     0x10        // EUSART TX

void _DEBUG_SerialIdentify(void)
{
    saved_baudrate = SP1BRG;
    saved_rx_pin = RXPPS;       
    saved_tx_pin = 0xFF;        // sentinel for no tx found
    // find the pin currently assigned as TX
    uint8_t i, *reg = (uint8_t*)&RA0PPS;
    for (i = 0; i < 24; i++) {
        if (*reg++ == TX_FUNCTION)
            saved_tx_pin = i;
    }

    // assign and configure pins to connect to USB bridge
    LATCbits.LATC0 = 1;         // set LATx to high
    TRISCbits.TRISC0 = 0;       // RC0 output
    TRISCbits.TRISC1 = 1;       // RC1 input
    ANSELCbits.ANSC1 = 0;       // RC1 digital
}

void _DEBUG_SerialCapture(void)
{
    // ensure any pending TX is completed
    while(0 == TX1STAbits.TRMT);
    // turn off TX?RX?
    // set debugging speed
    SP1BRG = (uint16_t) (((_XTAL_FREQ/4) / DEBUG_BAUDRATE)-1);
    // switch PPS to USB bridge
    RC0PPS = TX_FUNCTION;   // RC0->EUSART:TX;
    RXPPS  = DEBUG_RX_PIN;  // RC1->EUSART:RX;
    if (saved_tx_pin == 0xff) return; // tx was not used
    // disconnect output from application tx pin
    uint8_t port = saved_tx_pin >> 3; 
    uint8_t pin  = saved_tx_pin & 7;
    *(&LATA + port) |= 1 << pin;    // set LATx to high
    *(&RA0PPS + saved_tx_pin) = IO_FUNCTION; // return to gpio control
}

void _DEBUG_SerialRelease(void)
{
    // ensure any pending TX is completed
    while(0 == TX1STAbits.TRMT);
    // restore baudrate settings
    SP1BRG = saved_baudrate;
    // restore PPS settings
    *(&RA0PPS + saved_tx_pin) = TX_FUNCTION;   // app RXpin->EUSART:TX;
    RXPPS  = saved_rx_pin;   // RC1->EUSART:RX;
    // disconnect output from UART bridge tx pin
    LATCbits.LATC0 = 1;         // set LATx to high
    RC0PPS = IO_FUNCTION;       // return to gpio control
}
#endif

/**
 *  Use to initialize the UART when NOT already used by the application
 */
void _DEBUG_SerialInitialize(void)
{    
    // ABDOVF no_overflow; SCKP Non-Inverted; BRG16 16bit_generator; WUE disabled; ABDEN disabled; 
    BAUD1CON = 0x08;
    // SPEN enabled; RX9 8-bit; CREN enabled; ADDEN disabled; SREN disabled; 
    RC1STA = 0x90;
    // TX9 8-bit; TX9D 0; SENDB sync_break_complete; TXEN enabled; SYNC asynchronous; BRGH hi_speed; CSRC slave; 
    TX1STA = 0x24;
    // compute divider from _XTAL_FREQ to set Baud Rate = 115200
    SP1BRG = (uint16_t) (((_XTAL_FREQ/4) / DEBUG_BAUDRATE)-1);

    // assign and configure pins to connect to USB bridge
    LATCbits.LATC0 = 1;         // set LATx to high
    RC0PPS = 0x0010;            // RC0->EUSART:TX;
    TRISCbits.TRISC0 = 0;       // RC0 output
    RXPPSbits.RXPPS = 0x11;     // RC1->EUSART:RX;
    TRISCbits.TRISC1 = 1;       // RC1 input
    ANSELCbits.ANSC1 = 0;       // RC1 digital
}

char DEBUG_getch(void)
{
    while(!PIR3bits.RCIF);
    if(1 == RC1STAbits.OERR) {
        RC1STAbits.CREN = 0; 
        RC1STAbits.CREN = 1; 
    }
    return RC1REG;
}

void DEBUG_putch(uint8_t txData)
{
    while(0 == PIR3bits.TXIF);
    TX1REG = txData;    // Write the data byte to the USART.
}


// send a byte in asci(hex) to the serial port
void _LOG_BYTE( uint8_t b)
{
    char c = ((b >> 4) & 0x0f) + '0';    
    DEBUG_putch((c > '9') ? c + 7 : c);
    c = (b & 0x0f) + '0';
    DEBUG_putch((c > '9') ? c + 7 : c);
}

void _debug_puts(char*s)
{
    while(*s) DEBUG_putch(*s++);
    DEBUG_putch('\n');
}

// wait for user acknowledge, return char (absorb '/r')    
char _DEBUG_Ack(void)
{   char c;
    do{ 
        while(!EUSART_DataReady);   // wait 
        c = EUSART_Read();
        DEBUG_putch(c);            // echo
    } while (c == '\r');            // absorb
    return c;
}

// switch serial port, LEDs and User button
void DEBUG_Initialize(void)
{
#ifdef DEBUG_SHARE_SERIAL    
    _DEBUG_SerialIdentify();
#else
    _DEBUG_SerialInitialize();
#endif
#ifdef DEBUG_USE_LEDS
    _LED_TRIS &= ~_LED_MASK;         // make LED outputs
#endif
#ifdef DEBUG_USE_BUTTON
    _BUTTON_TRIS |= _BUTTON_MASK;   // make button input
    _BUTTON_ANSEL &= ~_BUTTON_MASK; // make button digital input
#endif
}

/** 
 * send a char to the debug serial port, 
 *  ex: LOG_CHAR('*');
 *  ->  *
 */
void LOG_CHAR(char x)     
{
__SERIAL_CAPTURE();
    DEBUG_putch((x)); 
__SERIAL_RELEASE();
}

/** 
 * send a value y in ascii (hex)) to the debug serial port, 
 *  ex: WATCH_BYTE('2', bcount);
 *  ->  @2:FF
*/
void WATCH_BYTE( char x, uint8_t y)
{ 
__SERIAL_CAPTURE();
    DEBUG_putch('@'); _LOG_BYTE(x); DEBUG_putch(':'); 
    _LOG_BYTE(y); DEBUG_putch('\n'); 
__SERIAL_RELEASE();
}

/** 
 * send a value y in ascii (hex)) to the debug serial port, 
 *  ex: WATCH_WORD('7', wcount);
 *  ->  @7:12AB
*/
void WATCH_WORD( char x, uint8_t w)  
{
__SERIAL_CAPTURE();
    DEBUG_putch('@'); _LOG_BYTE(x); DEBUG_putch(':'); 
    _LOG_BYTE((w)>>8);  _LOG_BYTE((w)); 
    DEBUG_putch('\n'); 
__SERIAL_RELEASE();
}

/** send nth breakpoint and wait for the user to send back a char
 *  ex: BREAK_NUM(0x17);
 *  ->  17>    
 */
void BREAK(uint8_t x) 
{ 
__SERIAL_CAPTURE();    
    DEBUG_putch('#');  _LOG_BYTE(x);
    DEBUG_putch('>');  _DEBUG_Ack(); 
__SERIAL_RELEASE();
 }

// display breakpoint number, then respond to console
void BREAK_DBG(uint8_t x)
{
    char s[DEBUG_STR_SIZE], *ptr, c;    // input command string
    const char *sep = ",: ";            // valid separators
    uint8_t len = 0;
    
__SERIAL_CAPTURE();    
    // show breakpoint # first 
    DEBUG_putch('#');  _LOG_BYTE(x);
    
    // main console loop
    while(1){ 
        // prompt
        DEBUG_putch('\n');
        DEBUG_putch(':');      
        // receive command string
        while(1) {
            c = _DEBUG_Ack();
            if (c == '\n') 
                break;
            else if ((c < ' ') || (c > '~')) 
                continue;
            else if (len < DEBUG_STR_SIZE-1) {         
                s[len++] = c; s[len] = '\0';
            }
        }
        // process command string
        ptr = strtok(s, sep);
        while(ptr != NULL) {
            switch(toupper(*ptr++)) {
            case 'R': // read a byte/word from address
                switch(toupper(*ptr)) {
                case 'B': // read byte
                    ptr = strtok(NULL, sep);
                    uint16_t addr = xtoi(ptr);  // get addr
                    _LOG_BYTE(*(uint8_t*)addr);
                    break;
                case 'W': // read word
                    ptr = strtok(NULL, sep);
                    uint16_t addr = xtoi(ptr);  // get addr
                    _LOG_BYTE(*(uint8_t*)addr);
                    _LOG_BYTE(*(uint8_t*)(addr+1));
                    break;
                default:   // command not recognized
                    DEBUG_putch('?');
//                    _debug_puts("->valid commands are: RB, RW");                        
                }
                break;
            case 'W': // write a byte/word to memory
                switch(toupper(*ptr)) {
                case 'B': // write byte
                    ptr = strtok(NULL, sep);
                    uint16_t addr = xtoi(ptr);  // get addr
                    ptr = strtok(NULL, sep);
                    uint8_t value = xtoi(ptr);  // get value
                    *(uint8_t*)addr = value;
                    DEBUG_putch('!');                    
                    break;
                case 'W': // write word
                    ptr = strtok(NULL, sep);
                    uint16_t addr = xtoi(ptr);  // get addr
                    ptr = strtok(NULL, sep);
                    uint8_t value = xtoi(ptr);  // get value
                    *(uint8_t*)addr = value;
                    DEBUG_putch('!');                    
                    break;
                default:   // command not recognized
                    DEBUG_putch('?');                    
//                    _debug_puts("->valid commands are: WB, WW");                        
                }
                break;
            case 'C': // continue executing from where you left
                __SERIAL_RELEASE();
                return;
            default:   // command not recognized
                DEBUG_putch('?');
//                _debug_puts("->valid commands are: R, W, C");
            }
            ptr = strtok(NULL, sep);
        } // switch
        len = 0;
    } // console loop
}

