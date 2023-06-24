/*
* main.c
*
* Created: 5/29/2023 12:53:59 AM
*  Author: wolf-
*/
#define F_CPU 16000000UL

#include <xc.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

// ������� ���������� �������-2 ~61 ���/�������

#define DELAY 1000
#define FLOW_INERTION 10 // ���������� ����� �������� ������ ������� ����
#define BTN_PRESS_SHORT 7
#define BTN_PRESS_LONG 200
#define BUZZER_SHORT_TIMER 30
#define BUZZER_LONG_TIMER 100
#define PUMP_TARGET_0 90 // 200ml
#define PUMP_TARGET_1 180 // 400ml
#define PUMP_TARGET_2 270 // 600ml
#define PUMP_TARGET_3 450 // 1000ml
#define PUMP_CHECK 180
#define EEPROM_TIMER 610

#define BTN_0 (!(PIND & (1 << 0)))
#define BTN_1 (!(PIND & (1 << 1)))
#define BTN_2 (!(PIND & (1 << 5)))
#define BTN_3 (!(PIND & (1 << 6)))
#define BTN_4 (!(PIND & (1 << 7)))
#define BUZZER_0 (PIND & (1 << 3))
#define PUMP_0 (PIND & (1 << 4))
#define REST 8505 // ������� ���� ������� (���-�� � ���������, 10 ��������� = 45��)
#define ML_PER_IMPULSE 0.22222222 // ���-�� ����������� �������� �� 1 ������� ��������


uint32_t rest = REST; // ������� ���� � ������� (�������)
uint32_t rest_eeprom = 0; // ������� ���� � ������� (���������� � ������ EEPROM)
uint8_t dots = 0b0100; // ����� ����� ���������� �� ������� (0, 1, 2, 3)
uint32_t pump_target = 0; // ���������� ��������� �������� ������� ������ ��������� �����
uint32_t pump_target_last = 0; // ��������� ���������� � EEPROM �������
uint8_t button_state = 0b00000000; // ����� ��������� ������
uint32_t button_press_counter = 0; // ������� ����, ������� ���� ������ ������� (��� ����������� �������� � ���������� �������)
uint8_t button_reg = 0b000; // 0 - ���� ���������� � ��������� ��������� ������� | 1 - ���� ������ ���������� (��� ������� ���� ��������)
uint32_t timer0 = 0; 
uint32_t timer2_pump = 0; // ������ ��� ����� (�������������� � ����� ���������� ������ ����)
uint8_t timer2_buzzer = 0; // ������ �������
uint32_t timer2_eeprom = 0; // ������ ������ � ������ (��� �� �������� ���������� ������ � ������ ��� ��������� ������� ����)


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

ISR(TIMER0_OVF_vect) // ���������� ������-0
{		
	button_press_counter++;
}

ISR(TIMER2_OVF_vect) { // ���������� ������-2	
	if (BUZZER_0 && timer2_buzzer > 0) {
		timer2_buzzer--;
	}
	
	if (PUMP_0 && timer2_pump > 0) {
		timer2_pump--;
	}
	
	if (timer2_eeprom > 0) {
		timer2_eeprom--;
	}
}

ISR(INT0_vect) // ���������� �������� ����
{	
	if (rest > 0) rest--;	
	if (pump_target > 0) pump_target--;	
}


void pump_on() {
	PORTD |= (1 << 4);
	dots |= (1 << 0);
}

void pump_off() {
	if (PUMP_0) beep_short();
	
	pump_target = 0;
	pump_target_last = 0;
	
	PORTD &= ~(1 << 4);
	dots &= ~(1 << 0);	
}

void beep_short() {
	timer2_buzzer = BUZZER_SHORT_TIMER;
}

void beep_long() {
	timer2_buzzer = BUZZER_LONG_TIMER;
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
	TIMSK |= (1 << TOIE2); // ������� ������������ ������� �������� 61 ���/�������
	
	GICR  |= (1 << INT0); // enable INT0;
	// ���������� �� ������������ ������
	MCUCR &= ~(1 << ISC00);
	MCUCR |= (1 << ISC01);
		
	sei(); // �������� ����������
	
	DDRB = 0xFF;
	DDRC = 0xFF;
	DDRD = 0b01111000;
	
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD = 0b11101011;
	
	eeprom_read_block(&rest, 0, sizeof(rest));
	rest_eeprom = rest;
	
	beep_short();
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
	init(); // �������������
	
	while (1) {
		// ���������� ������� ���� � ������, ���� �� �� ��� ������� �����
		if (!PUMP_0 && rest != rest_eeprom && timer2_eeprom == 0) {			
			rest_eeprom = rest;
			eeprom_update_block(&rest, 0, sizeof(rest));
			timer2_eeprom = EEPROM_TIMER; // ��������, ��� �� �������� ���������� ������ � ������ ��� ��������� �������� (�������� ����� ���������� ������)
		}
				
		if (timer2_pump > 0 || timer2_buzzer > 0 || timer2_eeprom > 0) { // �������� ������-2 ���� ���� ��� �������
			TCCR2 |= (1 << CS20) | (1 << CS21) | (1 << CS22);
		} else {
			TCCR2 &= ~(1 << CS20);
			TCCR2 &= ~(1 << CS21);
			TCCR2 &= ~(1 << CS22);
		}
		
		// ��������/��������� ������
		if (timer2_buzzer > 0) {
			PORTD |= (1 << PIND3);
		} else {
			PORTD &= ~(1 << PIND3);
		}
		
		keyboard(); // ��������� ������� ������
				
		if (PUMP_0 && pump_target > 0) { // ���� ����� ����� ��������
			display((pump_target + FLOW_INERTION) * ML_PER_IMPULSE * 10, 0b0001);
			
			if (pump_target != pump_target_last) {
				pump_target_last = pump_target;
				timer2_pump = PUMP_CHECK;
			}
			else if (timer2_pump == 0) {
				pump_off();
				beep_long();
			}
		} else {
			display(rest * ML_PER_IMPULSE, dots);
		}
		
		if (timer2_buzzer > 0) {
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
			button_reg &= ~(1 << 0); // ������� ���� ���������� � ��������� ��������� ������
			
			// ���� �������� ����� ��� ������ - ��������� � (������� ��� ������������)
			if (PUMP_0 || timer2_buzzer > 0) {
				pump_off();
				continue;
			}
			
			switch (button_state) {
				// �������� �������								
				case 0b00000001: // ������ 0
				pump_target = PUMP_TARGET_0 - FLOW_INERTION;
				break;
								
				case 0b00000010: // ������ 1
				pump_target = PUMP_TARGET_1 - FLOW_INERTION;
				break;
								
				case 0b00000100: // ������ 2
				pump_target = PUMP_TARGET_2 - FLOW_INERTION;
				break;
								
				case 0b00001000: // ������ 3
				pump_target = PUMP_TARGET_3 - FLOW_INERTION;
				break;
				
				// ���������� �������				
				case 0b10000001: // ������ 0
				
				break;
								
				case 0b10000010: // ������ 1
				
				break;
								
				case 0b10000100: // ������ 2
				
				break;
								
				case 0b10001000: // ������ 3
				
				break;
												
				case 0b10000011: // ������ 0+1
				rest = REST;
				beep_long();
				break;
			}
		}
	}
}