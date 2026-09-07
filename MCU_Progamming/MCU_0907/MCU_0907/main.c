#include "d1PM/board.h"
#include "d1PM/btn.h"
#include "d1PM/led.h"
#include "d1PM/fnd.h"
#include <util/delay.h>
#include <avr/interrupt.h>

static void pattern_sequential(){
	for (uint8_t n = 0; n < 8; n++) { 
		led_on(n); _delay_ms(200); 
		led_off(n);
	}	
}

static void pattern_pingpong(){
	for (uint8_t n = 0; n < 8; n++) {
		led_on(n); 
		_delay_ms(200);
		led_off(n);
	}
	
	for (uint8_t n = 8; n >0; n--) {
		led_on(n-1);
		_delay_ms(200);
		led_off(n-1);
	}
}


static void pattern_alternate(){
	led_write(0x55); _delay_ms(200);   // 짝수 켜기
	led_write(0xAA); _delay_ms(200);   // 홀수 켜기
}

volatile uint8_t mode =0;

ISR(INT4_vect){
	mode++;
	if(mode>=3) mode=0;
}

int main(void)
{
	uint16_t value = 1234;
	fnd_init();
	
	btn_init();
	led_init();
	EIMSK |= (1 << INT4);       // INT4 인터럽트 허용
	EICRB |= (1 << ISC41);      // 하강 에지(누름)에 반응
	sei();
	
	while (1) {
	// fnd랑 같이 띄우려면 타이머가 필요함. 
		
		switch (mode) {
			case 0: pattern_sequential(); break;
			case 1: pattern_pingpong(); break;
			case 2: pattern_alternate(); break;
		}
	}
}