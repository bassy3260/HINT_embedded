#include <avr/io.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lcd.h"

#define NUM_PRODUCERS	5
#define NUM_CUNSUMERS	5
#define QUEUE_SIZE 8
#define LED_ACTIVE_LOW 1

#define PRODUCER_MIN_DELAY 3000
#define PRODUCER_MAX_DELAY 8000

#define CONSUMER_MIN_DELAY 5000
#define CONSUMER_MAX_DELAY 7000
/* Queue Handle */
static QueueHandle_t xDataQueue;

/* 함수 선언 */
static void vProducerTask(void *pvParameters);
static void vConsumerTask(void *pvParameters);



static uint16_t random_number(uint16_t *seed);
static uint16_t random_delay(uint16_t *seed, uint16_t min, uint16_t max);
static uint8_t producer_id[NUM_PRODUCERS] = {0,1,2,3,4};
static uint8_t consumer_id[NUM_CUNSUMERS] = {0,1,2,3,4};

static void update_led(void)
{
	uint8_t n;

	n = uxQueueMessagesWaiting(xDataQueue);

	if(n == 0)
	PORTB = 0xFF;
	else if(n >= 8)
	PORTB = 0x00;
	else
	PORTB = ~((1 << n) - 1);
}
/*================================================
 * Producer Task
 *================================================*/
static void vProducerTask(void *pvParameters) {
    uint8_t data = 0;
	uint8_t id;
	uint16_t seed;
	uint16_t delay_time;

	id = *(uint8_t *)pvParameters;
	seed = 5678 + id;

    while (1) {
		
		delay_time = random_delay(&seed, PRODUCER_MIN_DELAY,PRODUCER_MAX_DELAY);
		vTaskDelay(pdMS_TO_TICKS(delay_time));
		
        xQueueSend(xDataQueue, &data, portMAX_DELAY);
		update_led();

       
    }
}

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
		n= uxQueueMessagesWaiting(xDataQueue);
		
		
		lcd_gotoxy(0,1);
		lcd_string("Count = ");
		
		
		if(n<=9){lcd_data('0'+n);}
		else{lcd_data('?');}
		
		lcd_string("   ");
		
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

/*================================================
 * Consumer Task
 *================================================*/
static void vConsumerTask(void *pvParameters) {
    uint8_t data;

	uint8_t id;
	uint16_t seed;
	uint16_t delay_time;

	id = *(uint8_t *)pvParameters;
	seed = 1234 + id;
    while (1) {
		
		delay_time = random_delay(&seed, CONSUMER_MIN_DELAY,CONSUMER_MAX_DELAY);
		
		vTaskDelay(pdMS_TO_TICKS(delay_time));
        /*
         * Queue에서 데이터 꺼내기
         *
         * Queue가 비어 있으면
         * 데이터가 들어올 때까지 기다림
         */
        if (xQueueReceive(xDataQueue, &data, portMAX_DELAY) == pdPASS) {

			update_led();   // ← 이것만 추가
        }

    }
}

/*================================================
 * main
 *================================================*/
int main(void) {
	uint8_t i;
	

    /*
     * LED 초기화
     */
    DDRB = 0xFF;

    /*
     * Active Low
     * 처음에는 모든 LED OFF
     */
    PORTB = 0xFF;

	lcd_init();
	lcd_clear();
    /*
     * Queue 생성
     *
     * uint8_t 데이터를 최대 8개 저장
     */
    xDataQueue = xQueueCreate(QUEUE_SIZE, sizeof(uint8_t));

    /*
     * Queue 생성 성공 확인
     */
    if (xDataQueue != NULL) {

       	for(i = 0;i<NUM_PRODUCERS;i++){
	       	xTaskCreate(vProducerTask,"Producer",80,&producer_id[i],1,NULL);
       	}
       	
       	for(i = 0; i<NUM_CUNSUMERS;i++){
	       	xTaskCreate(vConsumerTask,"Consumer",80,&consumer_id[i],1,NULL);
       	}
		  xTaskCreate(vLCDTask,"LCD",80,NULL,1,NULL);
        /*
         * Scheduler 시작
         */
        vTaskStartScheduler();
    }

    /*
     * 정상적으로 실행되면
     * 여기까지 오지 않음
     */
    while (1) {
    }

    return 0;
}