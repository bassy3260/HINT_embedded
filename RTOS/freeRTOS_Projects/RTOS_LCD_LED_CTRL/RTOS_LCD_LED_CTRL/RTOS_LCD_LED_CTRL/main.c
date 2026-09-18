/*
 * RTOS_LCD_LED_CTRL.c
 *
 * Created: 2026-09-18 오후 4:00:57
 * Author : User
 */ 
#define F_CPU 14745600UL
#include <avr/io.h>
#include <util/delay.h>
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "pwm.h"   

void vLEDBlinkTask(void *pvParameters){
	DDRB = 0xFF;
	PORTB = 0x00;
	
	for(;;){
		PORTB ^= 0xFF;
		
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void vLCDPrintTask(void *pvParameters){
	lcd_init();
	
	while(1){
		lcd_gotoxy(0,0);
		lcd_string("Atemega!!");
		
		lcd_gotoxy(0,1);
		lcd_string("Hello LDC!");
		
		_delay_ms(500);
		
		lcd_clear();
		
		_delay_ms(500);
	}
}

void vFANTask(void *pvParameters){
	const uint8_t speed[] = {0, 30, 60, 100};
	uint8_t i = 0;

	pwm_init();

	for(;;){
		pwm_set_duty(speed[i]);
		if(++i >= sizeof(speed)) i = 0;
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

int main(void)
{
	pwm_init();
	pwm_set_duty(100);   // 최고 속도로 계속 회전 (원하는 % 로)

	xTaskCreate(vLEDBlinkTask, "LED_TSK", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(vLCDPrintTask, "LCD_TSK", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	// FAN 태스크는 삭제

	vTaskStartScheduler();

	while (1) { }
}
