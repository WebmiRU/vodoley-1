/*
* main.c
*
* Created: 5/29/2023 12:53:59 AM
*  Author: wolf-
*/
#define F_CPU 1000000UL

#include <xc.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

/*#define TM1637_DELAY asm("nop");*/
#define TM1637_DELAY _delay_us(50);

#define DOT 0x80
#define DDR_0 DDRB
#define PORT_0 PORTB
#define DIO_0 2 // PORTD-0
#define CLK_0 1 //PORTD-1

uint8_t digits[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};

// Частота прерывания Таймера-2 ~61 раз/секунда

#define DELAY 1000
#define FLOW_INERTION 10 // Количество тиков счётчика потока энерции воды
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
#define REST 8505 // Остаток воды бутылке (кол-во в импульсах, 10 импульсов = 45мл)
#define ML_PER_IMPULSE 0.22222222 // Кол-во миллилитров жидкости на 1 импульс счётчика


uint32_t rest = REST; // Остаток воды в бутылке (емкости)
uint32_t rest_eeprom = 0; // Остаток воды в бутылке (записанный в память EEPROM)
uint8_t dots = 0b0100; // Какие точки отображать на дисплее (0, 1, 2, 3)
uint32_t pump_target = 0; // Количество импульсов счётчика которые должна прокачать помпа
uint32_t pump_target_last = 0; // Последнее записанное в EEPROM занение
uint8_t button_state = 0b00000000; // Флаги состояния кнопок
uint32_t button_press_counter = 0; // Счётчик того, сколько была нажата клавиша (для определения коротких и длительных нажатий)
uint8_t button_reg = 0b000; // 0 - флаг готовности к обработке сочетания клавишь | 1 - флаг сброса клавиатуры (все клавиши были отпущены)
uint32_t timer0 = 0; 
uint32_t timer2_pump = 0; // Таймер для помпы (автоотключение в случе отсутствия подачи воды)
uint8_t timer2_buzzer = 0; // Таймер зуммера
uint32_t timer2_eeprom = 0; // Таймер записи в память (что бы избежать постоянной записи в память при изменении остатка воды)

void TM1637_start() {
	DDR_0 |= (1 << CLK_0); // CLOCK
	DDR_0 |= (1 << DIO_0); // DATA
		
	PORT_0 |= (1 << CLK_0); // CLOCK
	PORT_0 |= (1 << DIO_0); // DATA
	TM1637_DELAY
		
	PORT_0 &= ~(1 << DIO_0); // DATA
	PORT_0 &= ~(1 << CLK_0); // CLOCK
	TM1637_DELAY		
}

void TM1637_stop() {	
	DDR_0 |= (1 << CLK_0); // CLOCK
	DDR_0 |= (1 << DIO_0); // DATA
		
	PORT_0 &= ~(1 << CLK_0); // CLOCK
	PORT_0 &= ~(1 << DIO_0); // DATA
	TM1637_DELAY
		
	PORT_0 |= (1 << CLK_0); // CLOCK
	PORT_0 |= (1 << DIO_0); // DATA
	TM1637_DELAY		
}

void TM1637_send_byte(uint8_t value) {	
	for (uint8_t i = 0; i < 8; i++) {
		PORT_0 &= ~(1 << CLK_0); // CLOCK=0
		TM1637_DELAY
			
		if ((value >> i) & 1) { // Передача байта
			PORT_0 |= (1 << DIO_0);
			} else {
			PORT_0 &= ~(1 << DIO_0);
		}
			
		TM1637_DELAY
		PORT_0 |= (1 << CLK_0); // CLOCK=1
		TM1637_DELAY
	}
		
	PORT_0 &= ~(1 << CLK_0); // CLOCK=0
	TM1637_DELAY
	PORT_0 |= (1 << CLK_0); // CLOCK=1
	TM1637_DELAY		
}

void TM1637_set_brightness(uint8_t brightness) { // 0-7
	if (brightness > 7) {
		brightness = 7;
	}
	
	TM1637_start();
	TM1637_send_byte(0x88 + brightness);
	TM1637_stop();
}

void TM1637_init(void) {
	TM1637_set_brightness(1); // Яркость дисплея 1
	
	TM1637_start();
	TM1637_send_byte(0xC0); // вывод с 1-го разряда.
	TM1637_send_byte(digits[7]);
	TM1637_send_byte(digits[2]);
	TM1637_send_byte(digits[9]);
	TM1637_send_byte(digits[9]);
	TM1637_stop();
}

void TM1637_number(uint32_t value, uint8_t dot) { // Берём число с запасом, для больших дисплеев
	TM1637_start();
	TM1637_send_byte(0xC0); // вывод с 1-го разряда.
	
	TM1637_send_byte(digits[value / 1000 % 10] + (dot == 1 ? DOT : 0));
	TM1637_send_byte(digits[value / 100 % 10] + (dot == 2 ? DOT : 0));
	TM1637_send_byte(digits[value / 10 % 10] + (dot == 3 ? DOT : 0));
	TM1637_send_byte(digits[value % 10] + (dot == 4 ? DOT : 0));

	TM1637_stop();
}

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

ISR(TIMER0_OVF_vect) // Прерывания Таймер-0
{		
	button_press_counter++;
}

ISR(TIMER2_OVF_vect) { // Прерывания Таймер-2	
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

ISR(INT0_vect) // Прерывание счётчика воды
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
	TIMSK |= (1 << TOIE2); // Частота срабатывания таймера примерно 61 раз/секунда
	
	GICR  |= (1 << INT0); // enable INT0;
	// Прерывание по ниспадающему фронту
	MCUCR &= ~(1 << ISC00);
	MCUCR |= (1 << ISC01);
		
	sei(); // Включаем прерывания
	
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

// 0-6 - Состояние клавишь | 7 - флаг длительного нажатия
void keyboard() {		
	if ((BTN_0 || BTN_1 || BTN_2 || BTN_3) && (button_reg & (1 << 1))) {
		button_state = 0x00;
		timer_buttons_on();
						
		if (BTN_0) button_state |= (1 << 0); 		
 		if (BTN_1) button_state |= (1 << 1);
 		if (BTN_2) button_state |= (1 << 2);
 		if (BTN_3) button_state |= (1 << 3);
	}
	
	// Короткое нажатие
	if(!BTN_0 && !BTN_1 && !BTN_2 && !BTN_3) {
		button_reg |= (1 << 1); // Флаг сброса клавиатуры (все клавиши были отпущены)
		
		if ((button_press_counter > BTN_PRESS_SHORT) && (button_press_counter < BTN_PRESS_LONG)) {									
			button_state &= ~(1 << 7); // Флаг длительного нажатия (снимаем)
			button_reg |= (1 << 0); // Флаг готовности к обработке состояниея клавишь
			button_reg &= ~(1 << 1); // Флаг сброса клавиатуры (все клавиши были отпущены)
		}
		
		timer_buttons_off();
						
	}
	// Длительное нажатие
	else if (button_press_counter > BTN_PRESS_LONG) {
		button_state |= (1 << 7); // Флаг длительного нажатия (устанавливаем)
		button_reg |= (1 << 0); // Флаг готовности к обработке
		button_reg &= ~(1 << 1); // Флаг сброса клавиатуры (все клавиши были отпущены)
		
		timer_buttons_off();
	}
}

int main(void)
{
	init(); // Инициализация
	
	TM1637_init();
	TM1637_number(8888, 2);
	for (uint16_t i = 0; i <= 9999; i++) {
		TM1637_number(i, 2);
		_delay_ms(10);
	}
	
	
	//while (1) {
		//// Записываем остаток воды в память, если он не был записан ранее
		//if (!PUMP_0 && rest != rest_eeprom && timer2_eeprom == 0) {			
			//rest_eeprom = rest;
			//eeprom_update_block(&rest, 0, sizeof(rest));
			//timer2_eeprom = EEPROM_TIMER; // Задержка, что бы избежать постоянной записи в память при изменении счётчика (экономим циклы перезаписи памяти)
		//}
				//
		//if (timer2_pump > 0 || timer2_buzzer > 0 || timer2_eeprom > 0) { // Включаем Таймер-2 если есть что считать
			//TCCR2 |= (1 << CS20) | (1 << CS21) | (1 << CS22);
		//} else {
			//TCCR2 &= ~(1 << CS20);
			//TCCR2 &= ~(1 << CS21);
			//TCCR2 &= ~(1 << CS22);
		//}
		//
		//// Включаем/выключаем зуммер
		//if (timer2_buzzer > 0) {
			//PORTD |= (1 << PIND3);
		//} else {
			//PORTD &= ~(1 << PIND3);
		//}
		//
		//keyboard(); // Обработка нажатия клавиш
				//
		//if (PUMP_0 && pump_target > 0) { // Если помпа помпа включена
			//display((pump_target + FLOW_INERTION) * ML_PER_IMPULSE * 10, 0b0001);
			//
			//if (pump_target != pump_target_last) {
				//pump_target_last = pump_target;
				//timer2_pump = PUMP_CHECK;
			//}
			//else if (timer2_pump == 0) {
				//pump_off();
				//beep_long();
			//}
		//} else {
			//display(rest * ML_PER_IMPULSE, dots);
		//}
		//
		//if (timer2_buzzer > 0) {
			//PORTD |= (1 << 3);
		//} else {
			//PORTD &= ~(1 << 3);
		//}		
				//
		//if (BTN_4) {
			//button_state |= (1 << 4);
			//pump_target = 0;
			//pump_on();			
		//}
		//else if (!BTN_4 && (button_state & (1 << 4))) {
			//button_state &= ~(1 << 4);
			//pump_target = 0;
			//pump_off();
		//}
		//else if (pump_target > 0) {
			//pump_on();
		//}
		//else {
			//pump_off();
		//}
		//
		//if (button_reg & (1 << 0)) {			
			//button_reg &= ~(1 << 0); // Снимаем флаг готовности к обработке состояния кнопок
			//
			//// Если включена помпа или зуммер - выключаем её (сделано для безопасности)
			//if (PUMP_0 || timer2_buzzer > 0) {
				//pump_off();
				//continue;
			//}
			//
			//switch (button_state) {
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
				//
				//// Длительные нажатия				
				//case 0b10000001: // Кнопка 0
				//
				//break;
								//
				//case 0b10000010: // Кнопка 1
				//
				//break;
								//
				//case 0b10000100: // Кнопка 2
				//
				//break;
								//
				//case 0b10001000: // Кнопка 3
				//
				//break;
												//
				//case 0b10000011: // Кнопка 0+1
				//rest = REST;
				//beep_long();
				//break;
			//}
		//}
	//}
}