
/*
 * lcd.h
 *
 * Created: 2026-09-18 오후 12:44:08
 *  Author: User
 */ 

#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#include <stdint.h>

#define RS				PG0
#define RW				PG1
#define E				PG2


#define CLEAR_DISPLAY	0
#define RETURN_HOME		1

#define ENTRY_MODE		2
#define E_ID			1
#define E_S				0

#define DISPLAY_ONFF	3
#define D_D				2
#define D_C				1
#define D_B				0

#define CURSOR_SHIFT	4
#define C_SC			3
#define C_RL			2

#define FUNCTION_SET    5
#define F_DL			4
#define F_N				3
#define F_F				2

#define SET_CGRAM		6
#define	SET_DDRAM		7

void lcd_init(void);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);


void lcd_clear(void);
void lcd_home(void);


void lcd_gotoxy(uint8_t x, uint8_t y);
void lcd_string(const char *str);

#endif