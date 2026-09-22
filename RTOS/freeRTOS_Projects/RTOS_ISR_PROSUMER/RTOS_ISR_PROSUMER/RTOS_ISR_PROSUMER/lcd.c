
/*
* lcd.c
*
* Created: 2026-09-18 오후 12:44:01
*  Author: User
*/
#ifndef F_CPU
#define F_CPU 14745600UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h"

static void lcd_busy(void){
	uint16_t busy;
	
	PORTA = 0x00;
	DDRA = 0x00;
	
	PORTG &= ~(1<<E);
	PORTG &= ~(1<<RS);
	PORTG |= (1<<RW);
	
	
	do{
		PORTG |= (1<<E);
		
		_delay_us(1);
		
		busy = PINA & 0x80;
		
		PORTG &= ~(1<<E);
		
		_delay_us(1);
		
	}while(busy);
	
	PORTG &= ~(1<<RW);
	
	DDRA = 0xFF;

	_delay_us(1);
   
}

static void lcd_epulse(void) {
	PORTG |= (1 << E);

	_delay_us(1);

	PORTG &= ~(1 << E);

	_delay_us(1);
}


void lcd_command(uint8_t cmd){
	lcd_busy();
	
	PORTA=cmd;
	
	PORTG &= ~(1<<RS);
	
	PORTG &= ~(1<<RW);
	lcd_epulse();
}

void lcd_data(uint8_t data){
	lcd_busy();
	
	PORTA = data;
	
	PORTG |= (1<<RS);
	
	PORTG &= ~(1<<RW);
	
	lcd_epulse();
}

void lcd_init(void){
	DDRA = 0xFF;
	
	DDRG |= (1<<RS) |
	(1<<RW) |
	(1<<E);
	
	PORTG &= ~((1<<RS) |
	(1<<RW) |
	(1<<E));
	
	_delay_ms(20);
	
	PORTA=0x30;
	
	lcd_epulse();
	
	_delay_ms(5);
	
	lcd_epulse();
	
	_delay_ms(200);
	
	lcd_epulse();
	
	
	_delay_us(200);
	
	lcd_command(
	(1<< FUNCTION_SET) |
	(1<<F_DL) |
	(1<<F_N)
	);
	
	lcd_command(1<<DISPLAY_ONFF);
	
	lcd_command(1<<CLEAR_DISPLAY);
	
	lcd_command(
	(1<<ENTRY_MODE)|
	(1<<E_ID)
	);
	
	lcd_command(
	(1<<DISPLAY_ONFF)|
	(1<<D_D) |
	(1<<D_C) |
	(1<<D_B)
	);
}
void lcd_clear(void){
	lcd_command(1<<CLEAR_DISPLAY);
}

void lcd_home(void){
	lcd_command(1<<RETURN_HOME);
}

void lcd_gotoxy(uint8_t x, uint8_t y){
	uint8_t address;
	
	if(y==0){
		address=x;
	}else{
		address = 0x40 +x;
	}
	
	lcd_command((1<<SET_DDRAM)|address);
}

void lcd_string(const char *str){
	while(*str != '\0'){
		lcd_data(*str);
		str++;
	}
}