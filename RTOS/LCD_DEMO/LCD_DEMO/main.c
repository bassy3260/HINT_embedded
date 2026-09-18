#define F_CPU 14745600UL

#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h"

int main(void)
{
	lcd_init();

	while (1)
	{
		lcd_gotoxy(0, 0);
		lcd_string("ATmega128");

		lcd_gotoxy(0, 1);
		lcd_string("Hello LCD!");

		_delay_ms(500);

		lcd_clear(); /* 화면 지우기 */

		_delay_ms(500);
	}

	return 0;
}