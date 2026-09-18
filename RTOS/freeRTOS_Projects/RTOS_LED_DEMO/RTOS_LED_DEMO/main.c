/*
 * RTOS_LED_DEMO.c
 *
 * Created: 2026-09-18 오후 1:57:32
 * Author : User
 */ 

// F_CPU : CPU 속도를 지정해준다
// CPU에 선이 16개 있으면 16bit..64면 64bit...
// 전기적 신호가 있으면 1없으면 0이야
// 주파수 대역: 1초에 데이터가 몇개들어가냐?
#define F_CPU 14745600UL
#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"

void vLEDBlinkTask(void *pvParameters){
	DDRB =0xFF;
	PORTB = 0x00;
	for(;;){
		PORTB ^= 0xFF;
		
		// 명령어을 만나면 프로세스가 다른 프로세스한테 양보
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}


int main(void)
{
	// 작업 공간을 만든다.
	// OS공간과 RTOS 공간이 분리된다.
	 
	xTaskCreate(vLEDBlinkTask, "LED_TSK",configMINIMAL_STACK_SIZE,NULL,1,NULL);


	vTaskStartScheduler();
    /* Replace with your application code */
    while (1) 
    {
    }
}

