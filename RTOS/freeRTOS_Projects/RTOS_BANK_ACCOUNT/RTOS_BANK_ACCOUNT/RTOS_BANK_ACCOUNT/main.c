/*
 * RTOS_BANK_ACCOUNT.c
 *
 * Created: 2026-09-21 오후 3:53:05
 * Author : User
 */ 

#include <avr/io.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"

#include "semphr.h"
#define INITIAL_BALANCE	1000
#define TRANSACTION_AMOUNT	100

volatile int16_t balance = INITIAL_BALANCE;

static void vDepositTask(void *pvParameters);
static void vWithdrawTask(void *pvParameters);
static void vLCDTask(void *pvParameters);

static void lcd_print_int16(int16_t value);

static SemaphoreHandle_t vbankSemaphore;

// 정수 LCD출력
static void lcd_print_int16(int16_t value){
	uint16_t v;
	uint16_t divisor;
	uint8_t started= 0;
	uint8_t digit;
	
	if(value<0){
		lcd_data('-');
		v=(uint16_t)(-value);
	}else{
		v=(uint16_t)value;
	}
	
	divisor= 10000;
	
	while(divisor >1){
		digit = v/divisor;
		
		if((digit !=0)||started){
			lcd_data('0' + digit);
			started =1;
		}
		
		v%= divisor;
		divisor /=10;
	}
	lcd_data('0'+v);
}

static void vDepositTask(void *pvParameters){
	
	int16_t temp;
	
	(void)pvParameters;
	
	// 모든 task가 거의 동시에 시작하도록 대기
	vTaskDelay(pdMS_TO_TICKS(2000));
	
	// 현재 잔액 읽기
	temp=balance;
	
	// race condition을 발생시키기 위한 의도적인 delay
	// 다른 task도 같은 balacne값을 읽음
	vTaskDelay(pdMS_TO_TICKS(500));
	xSemaphoreTake(vbankSemaphore,portMAX_DELAY);
	temp = balance;
	
	vTaskDelay(pdMS_TO_TICKS(500));

	temp += TRANSACTION_AMOUNT;
	

	balance = temp;
	xSemaphoreGive(vbankSemaphore);
	
	vTaskDelete(NULL);
}

static void vWithdrawTask(void *pvParameters){
	
	int16_t temp;
	
	(void)pvParameters;
	
	vTaskDelay(pdMS_TO_TICKS(2000));
	xSemaphoreTake(vbankSemaphore,portMAX_DELAY);
	temp=balance;
	
	vTaskDelay(pdMS_TO_TICKS(500));
	
	temp -= TRANSACTION_AMOUNT;
	
	balance = temp;
	xSemaphoreGive(vbankSemaphore);
	vTaskDelete(NULL);
}

static void vLCDTask(void *pvParameters){
	int16_t current_balance;
	
	(void)pvParameters;
	
	while(1){
	
		current_balance = balance;
		lcd_gotoxy(0,0);
		lcd_string("Bank Account");
		
		lcd_gotoxy(0,1);
		lcd_string("Balance: ");
		
		lcd_gotoxy(8,1);
		
		lcd_print_int16(current_balance);
		
		lcd_string(" ");

		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

int main(void)
{
	lcd_init();
	lcd_clear();
	
	vbankSemaphore=xSemaphoreCreateBinary();
	if(vbankSemaphore == NULL){
		PORTB =0x00;
		while(1);
	}
	xSemaphoreGive(vbankSemaphore);
	xTaskCreate(vDepositTask,"Deposit1", 80,NULL,1,NULL);
	xTaskCreate(vDepositTask,"Deposit2",80,NULL,1,NULL);
	
	xTaskCreate(vWithdrawTask,"Withdraw1", 80,NULL,1,NULL);
	xTaskCreate(vWithdrawTask,"Withdraw2",80,NULL,1,NULL);
	
	xTaskCreate(vLCDTask,"LCD",80,NULL,1,NULL);
	
	vTaskStartScheduler();
    /* Replace with your application code */
    while (1) 
   {
    }
}

