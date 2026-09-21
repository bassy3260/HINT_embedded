# include <avr/io.h>

#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "semphr.h"

static SemaphoreHandle_t xBufferSemaphore;

static void vTaskA(void *pvParameters){
	uint8_t i;
	
	(void)pvParameters;
	
	while(1){
		
		xSemaphoreTake(xBufferSemaphore,portMAX_DELAY);
		
		// 얘네도 전부 lcd 공유자원에 접근함
		lcd_clear();
		lcd_gotoxy(0,0);
		
		
		
		for(i=0; i<8; i++){
			lcd_data('A');
			
			vTaskDelay(pdMS_TO_TICKS(100));
		}
		xSemaphoreGive(xBufferSemaphore);
		vTaskDelay(pdMS_TO_TICKS(500));
		
	}
}

static void vTaskB(void *pvParamters){
	uint8_t i;
	
	(void)pvParamters;
	
	while(1){
		xSemaphoreTake(xBufferSemaphore,portMAX_DELAY);
		lcd_gotoxy(0,1);
		
		
		for(i=0;i<8;i++){
			lcd_data('B');
			vTaskDelay(pdMS_TO_TICKS(100));
		}
		xSemaphoreGive(xBufferSemaphore);
		vTaskDelay(pdMS_TO_TICKS(500));
		
	}
}


int main(void){
	lcd_init();
	lcd_clear();
	
	xBufferSemaphore = xSemaphoreCreateBinary();
	if(xBufferSemaphore == NULL){
		PORTB=0x00;
		while(1);
	}
	
	xSemaphoreGive(xBufferSemaphore);
	
	xTaskCreate(vTaskA,"TaskA",80,NULL,1,NULL);
	
	xTaskCreate(vTaskB,"TaskB",80,NULL,1,NULL);
	
	vTaskStartScheduler();
	
	while(1){}
}