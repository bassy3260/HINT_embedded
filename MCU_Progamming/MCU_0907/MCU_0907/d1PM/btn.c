#include <avr/io.h>
#include "board.h"
#include "btn.h"

void btn_init(void)
{
	BTN_DDR  &= ~BTN_MASK;
	BTN_PORT &= ~BTN_MASK;
}

uint8_t btn_is_pressed(uint8_t n)
{
	uint8_t bit = n - BTN_NUM_BASE + BTN_PIN_BASE;
	return (BTN_PIN & (1 << bit)) == 0;
}

uint8_t btn_is_falling(uint8_t n)
{
	static uint8_t prev[BTN_COUNT] = {0, 0, 0, 0};
	uint8_t idx  = n - BTN_NUM_BASE;
	uint8_t curr = btn_is_pressed(n);
	uint8_t edge = (prev[idx] == 0) && (curr == 1);
	prev[idx] = curr;
	return edge;
}