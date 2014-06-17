/********************************************************************************
Includes
********************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#ifdef	__cplusplus
extern "C" {
#endif
#include "usart.h"
#include "atmega128.h"

#ifdef	__cplusplus
}
#endif

//(OC3B/INT4) PE4

#define LG_ON_OFF_CMD   0b11110111000010001111101100000100
#define MELE_ON_OFF_CMD 0b10101000010101111001111100000000

#define MELE_START     (_eq1(highElapsedH, 8) && _eq1(lowElapsedH, 4))
#define MELE_LOW       (highElapsedH == 0 && lowElapsedH == 0)
#define MELE_HIGH      (highElapsedH == 0 && _eq2(lowElapsedH, 2))
#define MELE_STOP_1    (highElapsedH == 0 && _eq2(lowElapsedH, 40))
#define MELE_STOP_2    (highElapsedH == 0 && _eq2(lowElapsedH, 94))
#define MELE_STOP_3    (_eq1(highElapsedH, 8) && _eq1(lowElapsedH, 2))
#define BIG_DELAY      (_eq1(highElapsedH, 100) || _eq1(lowElapsedH, 100))

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)

// IR receiver GPIO
#define IR_IN_CONTROL_PORT         DDRE
#define IR_IN_CONTROL_PIN          DDE4
#define IR_IN_PORT                 PORTE
#define IR_IN_PIN                  PE4
#define IR_IN_INT_CTRL_TYPE_PORT   EICRB
#define IR_IN_INT_CTRL_TYPE_PIN0   ISC40
#define IR_IN_INT_CTRL_TYPE_PIN1   ISC41
#define IR_IN_INT_CTRL_ON_OFF_PORT EIMSK
#define IR_IN_INT_CTRL_ON_OFF_PIN  INT4

// IR transmitter GPIO
#define IR_OUT_CONTROL_PORT DDRD
#define IR_OUT_CONTROL_PIN  DDD6
#define IR_OUT_PORT         PORTD
#define IR_OUT_PIN          PD6

#define _between(value, min, max) ((value >= min) & (value <= max))
#define _eq1(value, expctedValue) _between(value, (expctedValue - 1), (expctedValue + 1))
#define _eq2(value, expctedValue) _between(value, (expctedValue - 2), (expctedValue + 2))

/********************************************************************************
Global Variables
********************************************************************************/
volatile uint8_t ovf_count = 0;
volatile uint8_t ir_flags = 0;
volatile uint16_t high_elapsed_time = 0;
volatile uint16_t low_elapsed_time = 0;
volatile uint64_t ir_cmd = 0;
volatile uint8_t cmd_count = 0;
volatile uint8_t usart_data = 10;
volatile uint8_t ir_out_cmd_count = 0;
volatile uint8_t ir_out_total_count = 0;

#define IR_ENABLED 0
#define RECEIVING  1
#define RECEIVED   2

#define GET_IR_FLAG(bit) (1 << bit) & ir_flags
#define SET_IR_FLAG(bit) ir_flags |= (1 << bit)
#define CLR_IR_FLAG(bit) ir_flags &= ~(1 << bit)

/********************************************************************************
Function Prototypes
********************************************************************************/

void process_ir_signal();
void init_ir_receiver();
void reset_ir_receiver();

void prepare_ir_signal();
void send_ir_cmd();
void reset_ir_transmitter();

/********************************************************************************
Interrupt Service
********************************************************************************/

ISR(INT4_vect)
{
	uint16_t t = TCNT1;
	TCNT1 = 0;
	ovf_count = 0;

	cli(); // disable all interrupts
	if (GET_IR_FLAG(IR_ENABLED)) {
		if (_read(PE4, PINE)) {
			high_elapsed_time = t;
			low_elapsed_time = 0;
		} else {
			low_elapsed_time = t;
			process_ir_signal();
		}
	} else {
		init_ir_receiver();
	}
	sei(); // enable all interrupts
}

ISR(TIMER1_OVF_vect)
{
	ovf_count++;
	reset_ir_receiver();
}

ISR(USART0_RX_vect)
{
	usart_data = UDR0;
}

ISR(TIMER1_COMPA_vect)
{
	cli(); // disable all interrupts
	prepare_ir_signal();
	sei(); // enable all interrupts
}

void process_ir_signal() {
	uint8_t highElapsedH = (uint8_t) (high_elapsed_time >> 8);
	//uint8_t highElapsedL = (uint8_t) (high_elapsed_time);

	uint8_t lowElapsedH = (uint8_t) (low_elapsed_time >> 8);
	//uint8_t lowElapsedL = (uint8_t) (low_elapsed_time);

	if (GET_IR_FLAG(RECEIVING)) {
		if (MELE_LOW) {
			cmd_count++;
		} else if (MELE_HIGH) {
			ir_cmd |= (((uint64_t)1) << cmd_count);
			cmd_count++;
		} else if (MELE_STOP_1 || MELE_STOP_2 || MELE_STOP_3) {
			CLR_IR_FLAG(RECEIVING);
			SET_IR_FLAG(RECEIVED);
		}
	} else if (MELE_START) {
		SET_IR_FLAG(RECEIVING);
		ir_cmd = 0;
		cmd_count = 0;
	}

	if (BIG_DELAY) {
		reset_ir_receiver();
	}
}

void init_ir_receiver() {
	SET_IR_FLAG(IR_ENABLED);
	_on(TOV1, TIFR); // reset overflow flag
	_on(TOIE1, TIMSK); // TOIE1 = 1: Timer/Counter1 overflow interrupt is enabled
}

void reset_ir_receiver() {
	_off(TOIE1, TIMSK); // TOIE1 = 0: Timer/Counter1 overflow interrupt is disabled
	CLR_IR_FLAG(IR_ENABLED);
	CLR_IR_FLAG(RECEIVING);
}

void enable_ir_receiver() {
    _on(IR_IN_INT_CTRL_ON_OFF_PIN, IR_IN_INT_CTRL_ON_OFF_PORT); // INT4 = 1: external pin interrupt is enabled.
}

void disable_ir_receiver() {
    _off(IR_IN_INT_CTRL_ON_OFF_PIN, IR_IN_INT_CTRL_ON_OFF_PORT); // INT4 = 1: external pin interrupt is enabled.
}

void prepare_ir_signal() {
	TCNT1 = 0;

	switch (ir_out_total_count) {
		// Preamble HIGH
		case 0:
			OCR1AH = 8;
			OCR1AL = 212;
			_on(IR_OUT_PIN, IR_OUT_PORT);
			break;
		// Preamble LOW
		case 1:
			OCR1AH = 4;
			OCR1AL = 95;
			_off(IR_OUT_PIN, IR_OUT_PORT);
			break;
		// End Marker 1 HIGH
		case 66:
			OCR1AH = 0;
			OCR1AL = 145;
			_on(IR_OUT_PIN, IR_OUT_PORT);
			break;
		// End Marker 1 LOW
		case 67:
			OCR1AH = 39;
			OCR1AL = 7;
			_off(IR_OUT_PIN, IR_OUT_PORT);
			break;
		// End Marker 2 HIGH
		case 68:
			OCR1AH = 8;
			OCR1AL = 215;
			_on(IR_OUT_PIN, IR_OUT_PORT);
			break;
		// End Marker 2 LOW
		case 69:
			OCR1AH = 2;
			OCR1AL = 40;
			_off(IR_OUT_PIN, IR_OUT_PORT);
			break;
		// End Marker 3 HIGH
		case 70:
			OCR1AH = 0;
			OCR1AL = 145;
			_on(IR_OUT_PIN, IR_OUT_PORT);
			break;

		default:
			if (ir_out_total_count < 66 ) {
				// Send IR command
				if (ir_out_total_count % 2 == 0) {
					OCR1AH = 0;
					OCR1AL = 145;
					_on(IR_OUT_PIN, IR_OUT_PORT);
				} else {
					if (ir_cmd & (((uint64_t)1) << ir_out_cmd_count++)) {
						OCR1AH = 1;
						OCR1AL = 145;
					} else {
						OCR1AH = 0;
						OCR1AL = 145;
					}
					_off(IR_OUT_PIN, IR_OUT_PORT);
				}
			} else {
				// Trasmission finished
				reset_ir_transmitter();
			}

			break;
	}

	ir_out_total_count++;
}

void send_ir_cmd() {
	ir_out_total_count = 0;
	ir_out_cmd_count = 0;

	TCNT1 = 0;
	// Initial timeout
	OCR1AH = 100;
	OCR1AL = 0;

	_on(OCF1A, TIFR); // reset overflow flag
	_on(OCIE1A, TIMSK); // Timer/Counter1 Output Compare A Match Interrupt is enabled.
}

void reset_ir_transmitter() {
	_off(OCIE1A, TIMSK); // OCIE1A = 0: Timer/Counter1 Output Compare A Match Interrupt is disabled.
	_off(IR_OUT_PIN, IR_OUT_PORT);

	enable_ir_receiver();
}

/********************************************************************************
Main
********************************************************************************/
int main(void) {
    // initialize code
	usart_init();

	// initialize input port for IR receiver
	_in(IR_IN_CONTROL_PIN,   IR_IN_CONTROL_PORT); // DDE4 = 0: PE4 is configured as input
	_on(IR_IN_PIN,           IR_IN_PORT); // PE4 = 1

	// initialize input port interrupt for IR receiver
	_on(IR_IN_INT_CTRL_TYPE_PIN0,  IR_IN_INT_CTRL_TYPE_PORT); // ISC40 = 1: Any logical change on INTn generates an interrupt request
	_off(IR_IN_INT_CTRL_TYPE_PIN1, IR_IN_INT_CTRL_TYPE_PORT); // ISC41 = 0: Any logical change on INTn generates an interrupt request
	_on(IR_IN_INT_CTRL_ON_OFF_PIN, IR_IN_INT_CTRL_ON_OFF_PORT); // INT4 = 1: external pin interrupt is enabled.

	// initialize output port for IR transmitter
	_out(IR_OUT_CONTROL_PIN, IR_OUT_CONTROL_PORT); // DDD6 = 1: PD6 is configured as output
	_off(IR_OUT_PIN,         IR_OUT_PORT); // PD6 = 1

	// initialize 16 bit timer 1
	TCCR1B = (0<<CS12)|(1<<CS11)|(1<<CS10); // CS12 = 0, CS11 = 1, CS10 = 1: clkI/O/64 (From prescaler)

    // enable interrupts
    sei();

    printf("INIT AFTER RESET");

    // main loop
    while (true) {
    	if (GET_IR_FLAG(RECEIVED)) {
    		CLR_IR_FLAG(RECEIVED);

    		if (((uint32_t)ir_cmd) == MELE_ON_OFF_CMD) {

    			disable_ir_receiver();

    			ir_cmd = (uint64_t) LG_ON_OFF_CMD;
    			send_ir_cmd();

    			printf ("X");
    		}

    		printf ("\n");
    	}
    }
}
