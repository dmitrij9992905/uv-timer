/*
 * main.c
 *
 *  Created on: 22.06.2015
 *      Author: Дмитрий
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"
#include <stdlib.h>
#include "ru.h"


uint16_t count = 0;
unsigned int backlight = 0;
int sec = 0;
int min = 0;
uint8_t timer = 0;
const int multiplier = 30;
uint8_t state = 0;
uint8_t flag = 0;
char integer[2];

void buzz(uint16_t duration) {
	for (uint16_t i = 0; i<duration; i++) {
		PORTD |= (1<<PD6);
		_delay_us(500);
		PORTD &= ~(1<<PD6);
		_delay_us(500);
	}
}

void LightOn();
void SoundPositive();
void SoundNegative();
void SoundEnd();

void timeIncrement() {
	cli();
	sec += multiplier;
	if(sec>59) {
		sec = 0;
		min++;
	}
	if (min > 99) {
		sec = 0;
		min = 0;
	}
	sei();
}

void timeDecrement() {
	cli();
	sec -= multiplier;
	if (sec < 0) {
		min--;
		sec = 59;
	}
	if (min < 0) {
		min = 99;
		sec = 59;
	}
	sei();
}

void init_IO() {
	DDRB |= (1 << 0);
	DDRD = 0b01000010;
	PORTB |= (1 << 0);
	PORTD = 0b00111101;
	GIMSK |= (1<<7);
	MCUCR &= ~(1 << 2); // Настройка срабатывания прерывания INT1 при падении (ХХХХ10ХХ)
	MCUCR |=  (1 << 3);
	MCUCR &= ~(1<<1); // Настройка срабатывания прерывания INT0 при любом изменении состояния
	MCUCR &= ~(1<<0);

	lcd_init(LCD_DISP_ON);
}

ISR(INT0_vect) {
		TCCR1B &= ~(1<<CS12);
		PORTD &= ~(1<<PD1);
		lcd_clrscr();
		while ((PIND&(1<<PD2)) == 0) {
			PORTB |= (1<<PB0);
			lcd_gotoxy(0,0);
			lcd_puts(CLOSE_LID);
			buzz(500);
			PORTB &= ~(1<<PB0);
			lcd_clrscr();
			_delay_ms(500);
		}
		TCCR1B |= (1<<CS12);
}

ISR(INT1_vect) {
	cli();
	LightOn();
	timer++;
	if (timer > 1) timer = 0;
	flag = 1;
	sei();
}

ISR(TIMER1_COMPA_vect) {
	cli();
	PORTD |= (1<<PD1);
	sec--;
	if (sec < 0) {
		min--;
		sec = 59;
	}
	if ((min == 0) && (sec <= 0)) {
		LightOn();
		TIMSK &= ~(1<<6);
		TCCR1B &= ~(1<<CS12);
		GIMSK &= ~(1<<6);
		PORTD &= ~(1<<PD1);
		min = 0;
		sec = 0;
		timer = 0;
		SoundEnd();
	}
	sei();
	TIFR = 0;
}

void TimeOutput() {
	lcd_gotoxy(0,1);
	lcd_puts("      :       ");
	lcd_gotoxy(4,1);
	itoa(min,integer, 10);
	lcd_puts(integer);
	lcd_gotoxy(7,1);
	itoa(sec,integer, 10);
	lcd_puts(integer);
}

void DisplayMode() {
	if (timer == 1) {
		lcd_gotoxy(0,0);
		lcd_puts(UV_LAMP);
	}
	else {
		lcd_gotoxy(0,0);
		lcd_puts(SET_TIME);
	}
}

void LightOn() {
	backlight = 0;
	PORTB |= (1<<PB0);
}

void TimeChange() {
	if (timer == 0) {
		if ((PIND&(1<<PD4)) == 0) {
			LightOn();
			timeIncrement();
			buzz(100);
		}
		if ((PIND&(1<<PD5))==0) {
			LightOn();
			timeDecrement();
			buzz(100);
		}
		if (((PIND&(1<<PD4)) == 0) && ((PIND&(1<<PD5))==0)) {
			LightOn();
			min = 0;
			sec = 0;
			SoundPositive();
		}
	}
	else if(((PIND&(1<<PD4)) == 0) || ((PIND&(1<<PD5))==0)) {
		SoundNegative();
	}
}

void SoundNegative() {
	buzz(100);
	_delay_ms(100);
	buzz(100);
}

void SoundPositive() {
	buzz(300);
}

void SoundEnd() {
	buzz(1000);
	_delay_ms(500);
	buzz(1000);
	_delay_ms(500);
	buzz(1000);
}

void Mode() {
	if (flag == 1) {
		cli();
		if (timer == 1) {
			if (min == 0 && sec == 0) {
				SoundEnd();
				timer = 0;
			}
			else {
				OCR1A = 0x7A12; //16-битный Timer1 настроен на сброс каждые 1 секунду
				TCCR1B |= (1 << WGM12);
				TCNT1 = 0;
				TIMSK |= (1<<6);
				TCCR1B |= (1 << WGM12);
				TCCR1B |= (1<<CS12);
				GIMSK |= (1<<6);
				PORTD |= (1<<PD1);
				SoundPositive();
			}
		}
		else {
			TIMSK &= ~(1<<6);
			TCCR1B &= ~(1<<CS12);
			GIMSK &= ~(1<<6);
			PORTD &= ~(1<<PD1);
			SoundNegative();
		}
		flag = 0;
		sei();
	}
}

int main() {
		if (state == 0) {
			init_IO();
			PORTD &= ~(1<<PD1);
			lcd_puts(HI);
			state = 1;
			_delay_ms(500);
			lcd_clrscr();
			sei();
		}
		while(1) {
			DisplayMode();
			TimeOutput();
			TimeChange();
			Mode();
			if(backlight < 20) backlight++;
			else PORTB &= ~(1<<PB0);
			_delay_ms(200);
		}

}
