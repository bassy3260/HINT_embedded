#include "d1PM/board.h"
#include "d1PM/led.h"
#include "d1PM/btn.h"

int main(void)
{
	uint8_t mode = 0;

	led_init();
	btn_init();

	while (1) {
		if (btn_is_falling(BTN_SW2)) {
			mode = !mode;
		}
		if (mode) led_on(0); else led_off(0);
	}
}