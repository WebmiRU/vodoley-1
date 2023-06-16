/*
* main.c
*
* Created: 5/29/2023 12:53:59 AM
*  Author: wolf-
*/
#define F_CPU 8000000UL

#include <xc.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define DELAY 1000
#define FLOW_INERTION 10
#define BTN_PRESS_SHORT 7
#define BTN_PRESS_LONG 200
#define BUZZER_BEEP_SHORT 25
#define BUZZER_BEEP_LONG 200
#define PUMP_TARGET_0 90 - FLOW_INERTION // 200ml
#define PUMP_TARGET_1 180 - FLOW_INERTION // 400ml
#define PUMP_TARGET_2 270 - FLOW_INERTION // 600ml
#define PUMP_TARGET_3 450 - FLOW_INERTION // 1000ml

#define BTN_0 (PIND & (1 << 0))
#define BTN_1 (PIND & (1 << 1))
#define BTN_2 (PIND & (1 << 5))
#define BTN_3 (PIND & (1 << 6))
#define BTN_4 (PIND & (1 << 7))
#define BUZZER_0 (PIND & (1 << 3))
#define PUMP_0 (PIND & (1 << 4))

uint32_t counter = 0;
uint8_t dots = 0b0100;
uint32_t pump_target = 0;
uint8_t buzzer_on_counter = 0;
//
//volatile uint8_t t0 = 0;
//#define T0_OVF 10

//uint32_t button_up = {0, 0, 0, 0, 0};

//uint32_t button_up[] = {0, 0, 0, 0, 0};
uint8_t button_down = 0b00000;
uint8_t button_up = 0b00000;
uint8_t button_state = 0b00000;
uint8_t button_press_short = 0b00000;
uint8_t button_press_reg = 0b00000;
uint32_t button_press_counter[] = {0, 0, 0, 0, 0};
uint32_t button_press_counter_global = 0;
	
uint32_t t1 = 1;

void display(uint32_t n, uint8_t dots)  {
	//uint32_t n = counter;
	
	for (int8_t i = 4 ; i > 0; i--) {
		uint8_t d = n % 10;
		
		PORTB |= (1 << 0);
		PORTB |= (1 << 3);
		PORTB |= (1 << 4);
		PORTC |= (1 << 0);
		PORTC &= ~(1 << 3); // POINT
		
		switch(d) {
			case 0:
			PORTB |= (1 << 1);
			PORTB |= (1 << 5);
			PORTC |= (1 << 2);
			PORTC |= (1 << 4);
			PORTC |= (1 << 5);
			PORTB |= (1 << 2);
			PORTC &= ~(1 << 1);
			break;
			
			case 1:
			PORTB &= ~(1 << 1);
			PORTB |= (1 << 5);
			PORTC |= (1 << 2);
			PORTC &= ~(1 << 4);
			PORTC &= ~(1 << 5);
			PORTB &= ~(1 << 2);
			PORTC &= ~(1 << 1);
			break;
			
			case 2:
			PORTB |= (1 << 1);
			PORTB |= (1 << 5);
			PORTC &= ~(1 << 2);
			PORTC |= (1 << 4);
			PORTC |= (1 << 5);
			PORTB &= ~(1 << 2);
			PORTC |= (1 << 1);
			break;
			
			case 3:
			PORTB |= (1 << 1);
			PORTB |= (1 << 5);
			PORTC |= (1 << 2);
			PORTC |= (1 << 4);
			PORTC &= ~(1 << 5);
			PORTB &= ~(1 << 2);
			PORTC |= (1 << 1);
			break;
			
			case 4:
			PORTB &= ~(1 << 1);
			PORTB |= (1 << 5);
			PORTC |= (1 << 2);
			PORTC &= ~(1 << 4);
			PORTC &= ~(1 << 5);
			PORTB |= (1 << 2);
			PORTC |= (1 << 1);
			break;
			
			case 5:
			PORTB |= (1 << 1);
			PORTB &= ~(1 << 5);
			PORTC |= (1 << 2);
			PORTC |= (1 << 4);
			PORTC &= ~(1 << 5);
			PORTB |= (1 << 2);
			PORTC |= (1 << 1);
			break;
			
			case 6:
			PORTB |= (1 << 1);
			PORTB &= ~(1 << 5);
			PORTC |= (1 << 2);
			PORTC |= (1 << 4);
			PORTC |= (1 << 5);
			PORTB |= (1 << 2);
			PORTC |= (1 << 1);
			break;
			
			case 7:
			PORTB |= (1 << 1);
			PORTB |= (1 << 5);
			PORTC |= (1 << 2);
			PORTC &= ~(1 << 4);
			PORTC &= ~(1 << 5);
			PORTB &= ~(1 << 2);
			PORTC &= ~(1 << 1);
			break;
			
			case 8:
			PORTB |= (1 << 1);
			PORTB |= (1 << 5);
			PORTC |= (1 << 2);
			PORTC |= (1 << 4);
			PORTC |= (1 << 5);
			PORTB |= (1 << 2);
			PORTC |= (1 << 1);
			break;
			
			case 9:
			PORTB |= (1 << 1);
			PORTB |= (1 << 5);
			PORTC |= (1 << 2);
			PORTC |= (1 << 4);
			PORTC &= ~(1 << 5);
			PORTB |= (1 << 2);
			PORTC |= (1 << 1);
			break;
		}
		
		switch(i) {
			case 1:
			PORTB &= ~(1 << 0);
			
			if (dots & (1 << 3)) {
				PORTC |= (1 << 3); // POINT
			}
			
			break;
			
			case 2:
			PORTB &= ~(1 << 3);
			//PORTC |= (1 << 3); // POINT
			if (dots & (1 << 2)) {
				PORTC |= (1 << 3); // POINT
			}
			break;
			
			case 3:
			PORTB &= ~(1 << 4);
			
			if (dots & (1 << 1)) {
				PORTC |= (1 << 3); // POINT
			}
			break;
			
			case 4:
			PORTC &= ~(1 << 0);
			
			if (dots & (1 << 0)) {
				PORTC |= (1 << 3); // POINT
			}
			break;
		}
		
		n /= 10;
	}
}

ISR(TIMER0_OVF_vect)
{
	if (buzzer_on_counter > 0) {
		buzzer_on_counter--;
	}
	
	if (BTN_0) { // Кнопка 0
		button_state &= ~(1 << 0);
	} else {
		button_state |= (1 << 0);
	}
	
	if (BTN_1) { // Кнопка 1
		button_state &= ~(1 << 1);
	}
	else {
		button_state |= (1 << 1);
	}
	
	if (BTN_2) { // Кнопка 2
		button_state &= ~(1 << 2);
	}
	else {
		button_state |= (1 << 2);
	}
	
	if (BTN_3) { // Кнопка 3
		button_state &= ~(1 << 3);
	}
	else {
		button_state |= (1 << 3);
	}
	
	if (BTN_4) { // Кнопка 4
		button_state &= ~(1 << 4);
	}
	else {
		button_state |= (1 << 4);
	}
	
	
	if (button_state == 0x00) {
		button_press_counter_global = 0;
	}
	else {
		button_press_counter_global++;		
	}
	
	for (uint8_t i = 0; i < 5; i++) {
		if (button_state & (1 << i)) {
			button_press_counter[i]++;
		}		
	}
	
	if (button_press_counter_global == BTN_PRESS_LONG) {
		button_press_reg |= (1 << 7); // Старший бит = 1, означет длительное нажатие кнопок
		
		for(uint8_t i = 0; i < 5; i++) {
			if (button_press_counter[i] > BTN_PRESS_SHORT) {
				button_press_reg |= (1 << i);				
			}
			else {
				button_press_reg &= ~(1 << i);				
			}						
		}
	}
	else if (button_state == 0x00) {		
		button_press_reg &= ~(1 << 7); // Старший бит = 0, означет короткое нажатие кнопок
		
		for (uint8_t i = 0; i < 5; i++) {				
			if ((button_press_counter[i] > BTN_PRESS_SHORT)) {
				if (button_press_counter[i] < BTN_PRESS_LONG) {
					button_press_reg |= (1 << i);
				}
				
				button_press_counter[i] = 0;
			} else {
				button_press_reg &= ~(1 << i);
			}
		}
	}
}

ISR(INT0_vect)
{
	counter++;
	
	if (pump_target > 0) {
		pump_target--;
	}
}


void pump_on() {
	PORTD |= (1 << 4);
	dots |= (1 << 0);
}

void pump_off() {
	PORTD &= ~(1 << 4);
	dots &= ~(1 << 0);
}

void beep_short() {
	buzzer_on_counter = BUZZER_BEEP_SHORT;
}

void beep_long() {
	buzzer_on_counter = BUZZER_BEEP_LONG;
}

void init() {
	TIMSK |= (1 << TOIE0);
	TCCR0 |= (1 << CS02); //|(1 << CS00);
	
	GICR  |= (1 << INT0); // enable INT0;
	MCUCR |= (1 << ISC01) | (1 << ISC00);
	sei();
	
	DDRB = 0xFF;
	DDRC = 0xFF;
	DDRD = 0b01111000;
	
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD = 0b11100011;
}

int main(void)
{
	init();	
	
	while (1) {
		display(counter, dots);
		
		//if (pump_target > 0) {
			//pump_on();
		//} else {
			//pump_off();
		//}
		
		if (buzzer_on_counter > 0) {
			PORTD |= (1 << 3);
		} else {
			PORTD &= ~(1 << 3);
		}		
		
		switch (button_press_reg) {
			// Длительные нажатия						
			case 0b10000001: // Кнопка 0
			counter+=100;
			break;
			
			case 0b10000010: // Кнопка 1
			counter+=200;
			break;						
			
			case 0b10000011: // Кнопки 0+1
			counter += 1000;
			break;
			
			case 0b10000101:  // Кнопки 0+2
			counter+=500;
			break;
			
			// Короткие нажатия
			case 0b00000001: // Кнопка 0
			counter+=1;
			break;
			
			case 0b00000010: // Кнопка 1
			counter+=2;
			break;
			
			case 0b00000100: // Кнопка 2
			counter+=3;
			break;
			
			case 0b00001000: // Кнопка 3
			counter+=4;
			break;
		}
		
		button_press_reg = 0;
	}
}