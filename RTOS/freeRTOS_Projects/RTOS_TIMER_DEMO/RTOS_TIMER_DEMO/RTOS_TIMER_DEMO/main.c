/*
 * RTOS_TIMER_DEMO.c
 *
 * Created: 2026-09-22 오후 4:18:50
 * Author : User
 */ 

#include <avr/io.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

static TimerHandle_t xLedTimer[8];

static void vLedTimerCallback(TimerHandle_t xTimer){
	uint8_t pin = (uint8_t)(uintptr_t)pvTimerGetTimerID(xTimer);
	
	PORTB ^= (1<< pin); // LED반전
}


int main(void)
{
	BaseType_t result;

	DDRB = 0xFF;
	
	PORTB = 0xFF;
	for(int i = 0; i<8; i++){
		xLedTimer[i] = xTimerCreate("LED Timer", pdMS_TO_TICKS((i + 1) * 1000),
					pdTRUE, (void *) i, vLedTimerCallback);
	}
	
	for(int i = 0; i<8 ;i++){
		if (xLedTimer[i] == NULL) {
			PORTB = 0x00;
			while (1) {}
		}
	
    /* Replace with your application code */
	}
	
	
	for (int i = 0; i < 8; i++){
		result = xTimerStart(xLedTimer[i], 0);
		if(result != pdPASS){
			PORTB = 0x00;
			while(1){};
		}
	}
	vTaskStartScheduler();

	while (1)
	{
	}
	return 0;
}

