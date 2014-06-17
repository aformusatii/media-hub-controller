/********************************************************************************
Includes
********************************************************************************/
#include "usart.h"

/********************************************************************************
Global Variables
********************************************************************************/
static FILE mystdout = FDEV_SETUP_STREAM(usart_putchar_printf, NULL, _FDEV_SETUP_WRITE);


void usart_init() {
    // Set baud rate
    UBRR0H = (uint8_t)((MYUBRR)>>8);
    UBRR0L = (uint8_t)(MYUBRR);

    // Enable receiver and transmitter and Interrupt on receive complete
    UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE);

    // Set frame format: 8data, 1stop bit
    UCSR0C = (1<<USBS0)|(3<<UCSZ00);

    // setup our stdio stream
    stdout = &mystdout;
}

void usart_putchar(char data) {
    // Wait for empty transmit buffer
    while ( !(UCSR0A & (_BV(UDRE0))) );

    // Start transmission
    UDR0 = data;
}

char usart_getchar(void) {
    // Wait for incoming data
    while ( !(UCSR0A & (_BV(RXC0))) );

    // Return the data
    return UDR0;
}

unsigned char usart_kbhit(void) {
    //return nonzero if char waiting polled version
    unsigned char b;
    b=0;
    if(UCSR0A & (1<<RXC0)) b=1;
    return b;
}

void usart_pstr(char *s) {
    // loop through entire string
    while (*s) {
        usart_putchar(*s);
        s++;
    }
}

// this function is called by printf as a stream handler
int usart_putchar_printf(char var, FILE *stream) {
    // translate \n to \r for br@y++ terminal
    if (var == '\n') usart_putchar('\r');
    usart_putchar(var);
    return 0;
}
