
#ifndef USART_H_
#define USART_H_

/********************************************************************************
Includes
********************************************************************************/
#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>

/********************************************************************************
Macros and Defines
********************************************************************************/
#define BAUD 19200
#define MYUBRR F_CPU/16/BAUD-1

/********************************************************************************
Function Prototypes
********************************************************************************/
void usart_init();
char usart_getchar( void );
void usart_putchar( char data );
void usart_pstr (char *s);
unsigned char usart_kbhit(void);
int usart_putchar_printf(char var, FILE *stream);

#endif /* USART_H_ */
