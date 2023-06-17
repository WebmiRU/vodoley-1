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
#define PUMP_TARGET_0 90 // 200ml
#define PUMP_TARGET_1 180 // 400ml
#define PUMP_TARGET_2 270 // 600ml
#define PUMP_TARGET_3 450 // 1000ml

#define BTN_0 (!(PIND & (1 << 0)))
#define BTN_1 (!(PIND & (1 << 1)))
#define BTN_2 (!(PIND & (1 << 5)))
#define BTN_3 (!(PIND & (1 << 6)))
#define BTN_4 (!(PIND & (1 << 7)))
#define BUZZER_0 (PIND & (1 << 3))
#define PUMP_0 (PIND & (1 << 4))
#define REST 8505 // Остаток воды бутылке (кол-во в импульсах)
#define ML_PER_IMPULSE 0.22222222 // Кол-во миллилитров жидкости на 1 импульс счётчика

uint32_t counter = 0;
uint32_t rest = REST; // Остаток воды в бутылке
uint8_t dots = 0b0100;
uint32_t pump_target = 0;
uint8_t buzzer_on_counter = 0;
//uint8_t button_down = 0b00000;
//uint8_t button_up = 0b00000;
uint8_t button_state = 0b00000000;
//uint8_t button_press_short = 0b00000;
//uint8_t button_press_reg = 0b00000;
//uint32_t button_press_counter[] = {0, 0, 0, 0, 0};
uint32_t button_press_counter_global = 0;
uint32_t timer0 = 0;
uint32_t button_press_counter = 0;
uint8_t button_reg = 0b000; // 0 - флаг готовности к обработке сочетания клавишь | 1 - флаг сброса клавиатуры (все клавиши были отпущены)

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
	
	button_press_counter++;
}

ISR(INT0_vect)
{	
	rest--;
	if (pump_target > 0) pump_target--;	
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

void timer0_on() {		
	TCCR0 |= (1 << CS02);
}

void timer0_off() {
	TCCR0 &= ~(1 << CS02);
	button_press_counter = 0;
}

void init() {
	TIMSK |= (1 << TOIE0);
	//TCCR0 |= (1 << CS02); //|(1 << CS00);
	
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

//void keyboard_press_counter_on() {
	//button_reg |= (1 << 1); // Включаем счётчик длительности нажатия кнопок
//}
//
//void keyboard_press_counter_off() {
	//button_reg &= ~(1 << 1); // Выключаем счётчик длительности нажатия кнопок
	//button_press_counter = 0;
//}

// 0-6 - Состояние клавишь | 7 - флаг длительного нажатия
void keyboard() {		
	if ((BTN_0 || BTN_1 || BTN_2 || BTN_3) && (button_reg & (1 << 1))) {
		timer0_on();
						
		if (BTN_0) button_state |= (1 << 0); 		
 		if (BTN_1) button_state |= (1 << 1);
 		if (BTN_2) button_state |= (1 << 2);
 		if (BTN_3) button_state |= (1 << 3);
	}
	
	// Короткое нажатие
	if(!BTN_0 && !BTN_1 && !BTN_2 && !BTN_3) {
		button_reg |= (1 << 1); // Флаг сброса клавиатуры (все клавиши были отпущены)
		
		if ((button_press_counter > BTN_PRESS_SHORT) && (button_press_counter < BTN_PRESS_LONG)) {									
			if (button_reg & (1 << 1)) {
				button_state &= ~(1 << 7); // Флаг длительного нажатия (снимаем)
				button_reg |= (1 << 0); // Флаг готовности к обработке состояниея клавишь
				button_reg &= ~(1 << 1); // Флаг сброса клавиатуры (все клавиши были отпущены)
			}
		}
		
		timer0_off();
						
	}
	// Длительное нажатие
	else if (button_press_counter > BTN_PRESS_LONG) {
		//if (button_reg & (1 << 1)) {
			button_state |= (1 << 7); // Флаг длительного нажатия (устанавливаем)
			button_reg |= (1 << 0); // Флаг готовности к обработке
			button_reg &= ~(1 << 1); // Флаг сброса клавиатуры (все клавиши были отпущены)
		//}				
		
		timer0_off();
	}
	
	//}
}

int main(void)
{
	init();	
	
	while (1) {
		keyboard();		
		
		if (button_reg & (1 << 1)) {
			pump_on();
		} else {
			pump_off();
		}
		
		//display(rest * ML_PER_IMPULSE, dots);
		//display(button_state, dots);
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
		
		if (button_reg & (1 << 0)) {
			//pump_on();
			//_delay_ms(300);
			//pump_off();
			
			switch (button_state) {
				// Короткие нажатия
				case 0b00000001:
				counter += 1;
				break;
				
				case 0b00000010:
				counter += 2;
				break;
				
				case 0b00000100:
				counter += 3;
				break;
				
				case 0b00001000:
				counter += 4;
				break;
				
				// Длительные нажатия
				case 0b10000001:
				counter += 100;
				break;
				
				case 0b10000010:
				counter += 200;
				break;
				
				case 0b10000100:
				counter += 300;
				break;
				
				case 0b10001000:
				counter += 400;
				break;
			}
			
			button_reg &= ~(1 << 0);
			button_state = 0;
		}
		
		//switch (button_press_reg) {
			//// Короткие нажатия
			//case 0b00000001: // Кнопка 0
			//pump_target = PUMP_TARGET_0 - FLOW_INERTION;
			//break;
			//
			//case 0b00000010: // Кнопка 1
			//pump_target = PUMP_TARGET_1 - FLOW_INERTION;
			//break;
			//
			//case 0b00000100: // Кнопка 2
			//pump_target = PUMP_TARGET_2 - FLOW_INERTION;
			//break;
			//
			//case 0b00001000: // Кнопка 3
			//pump_target = PUMP_TARGET_3 - FLOW_INERTION;
			//break;
		//}
		//
		//button_press_reg = 0;
	}
}