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
#define FLOW_INERTION 10 // ���������� ����� �������� ������ ������� ����
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
#define REST 8505 // ������� ���� ������� (���-�� � ���������)
#define ML_PER_IMPULSE 0.22222222 // ���-�� ����������� �������� �� 1 ������� ��������

uint32_t counter = 0;
uint32_t rest = REST; // ������� ���� � ������� (�������)
uint8_t dots = 0b0100;
uint32_t pump_target = 0;
uint8_t buzzer_on_counter = 0;
uint8_t button_state = 0b00000000;
uint32_t button_press_counter_global = 0;
uint32_t timer0 = 0;
uint32_t button_press_counter = 0;
uint8_t button_reg = 0b000; // 0 - ���� ���������� � ��������� ��������� ������� | 1 - ���� ������ ���������� (��� ������� ���� ��������)

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

ISR(INT0_vect) // ���������� ��������
{	
	if (rest > 0) rest--;	
	if (pump_target > 0) pump_target--;	
}


void pump_on() {
	PORTD |= (1 << 4);
	dots |= (1 << 0);
}

void pump_off() {
	pump_target = 0;
	PORTD &= ~(1 << 4);
	dots &= ~(1 << 0);	
}

void beep_short() {
	buzzer_on_counter = BUZZER_BEEP_SHORT;
}

void beep_long() {
	buzzer_on_counter = BUZZER_BEEP_LONG;
}

void timer_buttons_on() {		
	TCCR0 |= (1 << CS02);
}

void timer_buttons_off() {
	TCCR0 &= ~(1 << CS02);
	button_press_counter = 0;
}

void init() {
	TIMSK |= (1 << TOIE0);
	//TCCR0 |= (1 << CS02); //|(1 << CS00);
	
	GICR  |= (1 << INT0); // enable INT0;
	// ���������� �� ������������ ������
	MCUCR &= ~(1 << ISC00);
	MCUCR |= (1 << ISC01);
	sei();
	
	DDRB = 0xFF;
	DDRC = 0xFF;
	DDRD = 0b01111000;
	
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD = 0b11101011;
}

// 0-6 - ��������� ������� | 7 - ���� ����������� �������
void keyboard() {		
	if ((BTN_0 || BTN_1 || BTN_2 || BTN_3) && (button_reg & (1 << 1))) {
		button_state = 0x00;
		timer_buttons_on();
						
		if (BTN_0) button_state |= (1 << 0); 		
 		if (BTN_1) button_state |= (1 << 1);
 		if (BTN_2) button_state |= (1 << 2);
 		if (BTN_3) button_state |= (1 << 3);
	}
	
	// �������� �������
	if(!BTN_0 && !BTN_1 && !BTN_2 && !BTN_3) {
		button_reg |= (1 << 1); // ���� ������ ���������� (��� ������� ���� ��������)
		
		if ((button_press_counter > BTN_PRESS_SHORT) && (button_press_counter < BTN_PRESS_LONG)) {									
			button_state &= ~(1 << 7); // ���� ����������� ������� (�������)
			button_reg |= (1 << 0); // ���� ���������� � ��������� ���������� �������
			button_reg &= ~(1 << 1); // ���� ������ ���������� (��� ������� ���� ��������)
		}
		
		timer_buttons_off();
						
	}
	// ���������� �������
	else if (button_press_counter > BTN_PRESS_LONG) {
		button_state |= (1 << 7); // ���� ����������� ������� (�������������)
		button_reg |= (1 << 0); // ���� ���������� � ���������
		button_reg &= ~(1 << 1); // ���� ������ ���������� (��� ������� ���� ��������)
		
		timer_buttons_off();
	}
}

int main(void)
{
	init();	
	
	while (1) {
		keyboard();		
		
		if(PUMP_0 && pump_target > 0) {
			display((pump_target + FLOW_INERTION) * ML_PER_IMPULSE * 10, 0b0001);
		} else {
			display(rest * ML_PER_IMPULSE, dots);
		}		
		//display(button_state, dots);
		//display(counter, dots);
		
		if (buzzer_on_counter > 0) {
			PORTD |= (1 << 3);
		} else {
			PORTD &= ~(1 << 3);
		}
				
		if (BTN_4) {
			button_state |= (1 << 4);
			pump_target = 0;
			pump_on();			
		}
		else if (!BTN_4 && (button_state & (1 << 4))) {
			button_state &= ~(1 << 4);
			pump_target = 0;
			pump_off();
		}
		else if (pump_target > 0) {
			pump_on();
		}
		else {
			pump_off();
		}
		
		if (button_reg & (1 << 0)) {
			//pump_on();
			//_delay_ms(300);
			//pump_off();
			
			button_reg &= ~(1 << 0); // ������� ���� ���������� � ��������� ��������� ������
			
			if(PUMP_0) {
				pump_off();
				continue;
			}

			
			switch (button_state) {
				// �������� �������				
				// ������ 0
				case 0b00000001:
				pump_target = PUMP_TARGET_0 - FLOW_INERTION;
				break;
				
				// ������ 1
				case 0b00000010:
				pump_target = PUMP_TARGET_1 - FLOW_INERTION;
				break;
				
				// ������ 2
				case 0b00000100:
				pump_target = PUMP_TARGET_2 - FLOW_INERTION;
				break;
				
				// ������ 3
				case 0b00001000:
				pump_target = PUMP_TARGET_3 - FLOW_INERTION;
				break;
				
				// ���������� �������
				// ������ 0
				case 0b10000001:
				
				break;
				
				// ������ 1
				case 0b10000010:
				
				break;
				
				// ������ 2
				case 0b10000100:
				
				break;
				
				// ������ 3
				case 0b10001000:
				
				break;
								
				// ������ 0+1
				case 0b10000011:
				rest = REST;
				break;
			}
			
			//button_reg &= ~(1 << 0); // ������� ���� ���������� � ��������� ��������� ������
		}
	}
}