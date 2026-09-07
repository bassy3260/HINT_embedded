#include "d1PM/board.h"
#include "d1PM/buzzer.h"
#include "d1PM/btn.h"
#include <util/delay.h>

int main(void)
{
	
	uint8_t on = 0;
	
	btn_init();
	buzzer_init();

	while (1) {
		if(btn_is_falling(BTN_SW2)){
			on =!on;
		}
		if(on) buzzer_on(); else buzzer_off();
	}
}