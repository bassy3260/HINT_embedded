/*
 * RTOS_PROSUMER.c
 *
 * Created: 2026-09-21 오전 11:22:19
 * Author : User
 */ 

#include <avr/io.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"

#define BUFFER_SIZE 8

#define NUM_PRODUCERS	5
#define NUM_CUNSUMERS	5

#define LED_ACTIVE_LOW	1

#define PRODUCER_MIN_DELAY	2000
#define PRODUCER_MAX_DELAY	4000

#define CONSUMER_MIN_DELAY	4000
#define CONSUMER_MAX_DELAY	7000
/************************************************************************
*공유 버퍼
*
* 세마포어/Mutex를 사용하지 않음                                                            
************************************************************************/

volatile uint8_t buffer[BUFFER_SIZE];

volatile uint8_t head = 0;
volatile uint8_t tail = 0;
volatile uint8_t count = 0;

static uint8_t producer_id[NUM_PRODUCERS] = {0,1,2,3,4};
static uint8_t consumer_id[NUM_CUNSUMERS] = {0,1,2,3,4};


/************************************************************************
* 간단한 난수 생성기                                                                    
************************************************************************/

static uint16_t random_number(uint16_t *seed){
	*seed = (*seed *25173U) +13849U;
	
	return *seed;
}

/************************************************************************
* 랜덤 대기 시간
* min~max ms*/
static uint16_t random_delay(uint16_t *seed, uint16_t min, uint16_t max){
	uint16_t range;
	range = max-min+1;
	return min+(random_number(seed)%range);
}

static void vLCDTask(void *pvParameters){
	uint8_t n;
	(void)pvParameters;
	
	while(1){
		n=count;
		lcd_gotoxy(0,1);
		lcd_string("Count = ");
		
		if(n<=9){lcd_data('0'+n);}
		else{lcd_data('?');}
			
		lcd_string("   ");
		
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

static void update_led(void){
	uint8_t pattern;
	uint8_t n;
	
	n=count;
	
	if(n==0){pattern = 0x00;}
	else if(n>=8) {pattern=0xFF;}
	else {pattern = (1<<n)-1; }

#if LED_ACTIVE_LOW
	PORTB = ~pattern;
#else 
	PORTB = pattern;
#endif
}

/************************************************************************
* Producer      
************************************************************************/
static void vProducerTask(void *pvParameters){
	uint8_t id;
	uint8_t data = 0;

	uint8_t temp_count;
	uint8_t temp_head;
	
	uint16_t seed;
	uint16_t delay_time;
	
	id = *((uint8_t *)pvParameters);
	
	seed = 1000 +((uint16_t)id * 3000);
	
	while(1){
		/************************************************************************
		* 랜덤 시간 대기                                                                  
		************************************************************************/
		delay_time = random_delay(&seed, PRODUCER_MIN_DELAY,PRODUCER_MAX_DELAY);
		vTaskDelay(pdMS_TO_TICKS(delay_time));
		
		/************************************************************************
		* 버퍼가 꽉 찼는지 확인
	    * 문제:
		* 여러 Producer가 동시에
		* count < BUFFER_SIZE를 확인할 수 있음.
		************************************************************************/
		
		if(count<BUFFER_SIZE){
			data++;
			
			/************************************************************************
			* head읽기                                                                    
			************************************************************************/
			temp_head = head;
			
			/************************************************************************
			* Race condition을 잘 발생시키 위해 의도적으로 context switch
			************************************************************************/
			
			taskYIELD();
			
			/************************************************************************
			* 데이터 저장
			* 여러 Producer가 같은 위치에 동시에 쓸 수도 있음 
			************************************************************************/
			buffer[temp_head]=(id*10) +data;
			
			taskYIELD();
			
			/************************************************************************
			* head 증가
			************************************************************************/
			head = (temp_head + 1)% BUFFER_SIZE;
			
			/************************************************************************
			* count 증가
			 read -> modify -> write 이 과정이 원자적이지 않다.
			 */	
			temp_count = count;
			
			vTaskDelay(pdMS_TO_TICKS(500));
			
			temp_count++;
			
			vTaskDelay(pdMS_TO_TICKS(500));
			
			count = temp_count; // 다시 저장
			
			update_led();
		}
	
	}
}

/************************************************************************
* Consumer                         
************************************************************************/
static void vConsumerTask(void *pvParameters){
	uint8_t id;
	
	uint8_t data;
	
	uint8_t temp_count;
	uint8_t temp_tail;
	
	uint16_t seed;
	uint16_t delay_time;
	
	id=*((uint8_t *)pvParameters);
	
	seed = 30000 + ((uint16_t)id * 2000);
	
	while(1){
		/************************************************************************
		* 랜덤 시간 대기
		************************************************************************/
		delay_time = random_delay(&seed, CONSUMER_MIN_DELAY,CONSUMER_MAX_DELAY);
		
		vTaskDelay(pdMS_TO_TICKS(delay_time));
		
		/************************************************************************
		* 데이터가 있는지 확인
		* 문제: 여러 Consumer가 동시에
		* count> 0 을 확인할 수 있음
		************************************************************************/
		if (count> 0){
			/************************************************************************
			* tail 읽기 
			************************************************************************/
			temp_tail = tail;
			
			/************************************************************************
			* Race Condition유도 
			************************************************************************/
			taskYIELD();
		
			/************************************************************************
			*  데이터 읽기, 두 Consumet가 같은 데이터를 읽을 수도 있음                                                                  
			************************************************************************/
			data = buffer[temp_tail];
			
			(void)data;
			
			taskYIELD();
			
			tail = (temp_tail +1)%BUFFER_SIZE;
			
			temp_count = count;
			
			vTaskDelay(pdMS_TO_TICKS(500));
			temp_count--;
			vTaskDelay(pdMS_TO_TICKS(500));
			count = temp_count;
			update_led();
		}
	}
}

int main(void)
{
	
	uint8_t i;
	
	DDRB = 0xFF;
	PORTB = 0xFF;
	
	lcd_init();
	lcd_clear();
	
	lcd_gotoxy(0,0);
	lcd_string("Buffer Count:");
	
	
	for(i = 0;i<NUM_PRODUCERS;i++){
		xTaskCreate(vProducerTask,"Producer",80,&producer_id[i],1,NULL);
	}
	
	for(i = 0; i<NUM_CUNSUMERS;i++){
		xTaskCreate(vConsumerTask,"Consumer",80,&consumer_id[i],1,NULL);
	}
	xTaskCreate(vLCDTask,"LCD",80,NULL,1,NULL);
	
//	if(result !=pdPASS){
//		PORTB=0X00;
//		while(1);
//	}
	vTaskStartScheduler();
    /* Replace with your application code */
    while (1) 
    {
    }
}

