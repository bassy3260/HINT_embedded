/*
 * RTOS_LCD_DEMO.c
 *
 * Created: 2026-09-18 오후 3:16:21
 * Author : User
 */ 
#define F_CPU 14745600UL
#include <avr/io.h>
#include <util/delay.h>
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"

void vLCDPrintTask(void *pvParameters){
	lcd_init();
	
	while(1){
		lcd_gotoxy(0,0);
		lcd_string("Atmega128");
		
		lcd_gotoxy(0,1);
		lcd_string("Hello LCD!");
		
		_delay_ms(500);
		
		lcd_clear();
		
		_delay_ms(500);
		
	}
}

int main(void)
{
	
	xTaskCreate(vLCDPrintTask,"LCD_TSK",configMINIMAL_STACK_SIZE,NULL,1,NULL);
    /* Replace with your application code */
	
	vTaskStartScheduler();
    while (1) 
    {
    }
}

