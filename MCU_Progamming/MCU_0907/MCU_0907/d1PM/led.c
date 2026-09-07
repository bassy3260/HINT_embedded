#include <avr/io.h>
#include "board.h"
#include "led.h"

void led_init(void)
{
	LED_PORT = 0xFF;   /* 먼저 다 끄고 (Active-Low라 1=소등) */
	LED_DDR  = 0xFF;   /* 8핀 전부 출력으로 */
}
void led_write(uint8_t value) { LED_PORT = ~value; } 
void led_on(uint8_t n)     { LED_PORT &= ~(1 << n); }   /* 0이 점등 */
void led_off(uint8_t n)    { LED_PORT |=  (1 << n); }
void led_toggle(uint8_t n) { LED_PORT ^=  (1 << n); }