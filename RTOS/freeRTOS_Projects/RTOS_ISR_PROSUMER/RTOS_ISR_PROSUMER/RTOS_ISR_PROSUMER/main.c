/*
 * RTOS_ISR_PROSUMER.c
 *
 * Created: 2026-09-22 오전 5:34:16
 * Author : IncheolShin
 */

#include <avr/io.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"

#define BUFFER_SIZE          8

#define NUM_PRODUCERS        5
#define NUM_CONSUMERS        5

#define LED_ACTIVE_LOW       1

#define PRODUCER_MIN_DELAY   2000
#define PRODUCER_MAX_DELAY   4000

#define CONSUMER_MIN_DELAY   2000
#define CONSUMER_MAX_DELAY   4000

/*==============================================
 * 공유 버퍼
 *==============================================*/
volatile uint8_t buffer[BUFFER_SIZE];

volatile uint8_t head  = 0;
volatile uint8_t tail  = 0;
volatile uint8_t count = 0;


/* Producer / Consumer ID */
static uint8_t producer_id[NUM_PRODUCERS] = {0, 1, 2, 3, 4};
static uint8_t consumer_id[NUM_CONSUMERS] = {0, 1, 2, 3, 4};

/*==============================================
 * 간단한 난수 발생기
 *==============================================*/
static uint16_t random_number(uint16_t *seed) {
    *seed = (*seed * 25173U) + 13849U;

    return *seed;
}


/*==============================================
 * min ~ max ms 랜덤 시간
 *==============================================*/
static uint16_t random_delay(uint16_t *seed, uint16_t min, uint16_t max) {
    uint16_t range;

    range = max - min + 1;

    return min + (random_number(seed) % range);
}


/*==============================================
 * LED 표시
 *
 * count 만큼 LED ON
 *==============================================*/
static void update_led(void) {
    uint8_t pattern;
    uint8_t n;

    n = count;

    if (n == 0) { pattern = 0x00; }
    else if (n >= 8) { pattern = 0xFF; }
    else { pattern = (1U << n) - 1U; }

#if LED_ACTIVE_LOW
    PORTB = ~pattern;
#else
    PORTB = pattern;
#endif
}


/*==============================================
 * LCD Task
 *==============================================*/
static void vLCDTask(void *pvParameters) {
    uint8_t n;

    (void)pvParameters;

    while (1) {
        /*----------------------------------------
         * Critical Section
         *
         * count를 읽는 동안 인터럽트 금지
         *----------------------------------------*/
        taskENTER_CRITICAL();

        n = count;

        taskEXIT_CRITICAL();

        /*----------------------------------------
         * LCD 출력
         *
         * LCD 출력은 Critical Section 밖에서 수행
         *----------------------------------------*/
        lcd_gotoxy(0, 1);

        lcd_string("Count = ");

        if (n <= 9) { lcd_data('0' + n); }
        else { lcd_data('?'); }

        lcd_string("   ");

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


/*==============================================
 * Producer
 *==============================================*/
static void vProducerTask(void *pvParameters) {
    uint8_t id;
    uint8_t data = 0;

    uint8_t temp_count;
    uint8_t temp_head;

    uint16_t seed;
    uint16_t delay_time;

    id = *((uint8_t *)pvParameters);

    seed = 1000U + ((uint16_t)id * 3000U);

    while (1) {
        /*----------------------------------------
         * 랜덤 시간 대기
         *----------------------------------------*/
        delay_time = random_delay(&seed, PRODUCER_MIN_DELAY, PRODUCER_MAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(delay_time));

        data++;

        /*==============================================
         * Critical Section 시작
         *
         * 인터럽트가 비활성화됨
         *
         * 다른 Task가 중간에 끼어들 수 없음
         *==============================================*/
        taskENTER_CRITICAL();

        if (count < BUFFER_SIZE) {
            /*------------------------------------
             * head 읽기
             *------------------------------------*/
            temp_head = head;

            /*------------------------------------
             * 데이터 저장
             *------------------------------------*/
            buffer[temp_head] = (id * 10) + data;

            /*------------------------------------
             * head 증가
             *------------------------------------*/
            head = (temp_head + 1) % BUFFER_SIZE;

            /*------------------------------------
             * count 증가
             *
             * Read → Modify → Write
             * 전체가 Critical Section 내부
             *------------------------------------*/
            temp_count = count;

            temp_count++;

            count = temp_count;

            /* LED 갱신 */
            update_led();
        }

        /*==============================================
         * Critical Section 종료
         *
         * 인터럽트 다시 허용
         *==============================================*/
        taskEXIT_CRITICAL();
    }
}


/*==============================================
 * Consumer
 *==============================================*/
static void vConsumerTask(void *pvParameters) {
    uint8_t id;
    uint8_t data;

    uint8_t temp_count;
    uint8_t temp_tail;

    uint16_t seed;
    uint16_t delay_time;

    id = *((uint8_t *)pvParameters);

    seed = 30000U + ((uint16_t)id * 2000U);

    while (1) {
        /*----------------------------------------
         * 랜덤 시간 대기
         *----------------------------------------*/
        delay_time = random_delay(&seed, CONSUMER_MIN_DELAY, CONSUMER_MAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(delay_time));

        /*==============================================
         * Critical Section 시작
         *==============================================*/
        taskENTER_CRITICAL();

        if (count > 0) {
            /*------------------------------------
             * tail 읽기
             *------------------------------------*/
            temp_tail = tail;

            /*------------------------------------
             * 데이터 읽기
             *------------------------------------*/
            data = buffer[temp_tail];

            (void)data;

            /*------------------------------------
             * tail 증가
             *------------------------------------*/
            tail = (temp_tail + 1) % BUFFER_SIZE;

            /*------------------------------------
             * count 감소
             *------------------------------------*/
            temp_count = count;

            temp_count--;

            count = temp_count;


            update_led();
        }


        /*==============================================
         * Critical Section 종료
         *==============================================*/
        taskEXIT_CRITICAL();
    }
}

/*==============================================
 * main
 *==============================================*/
int main(void)
{
    uint8_t i;
    BaseType_t result;

    /*==============================================
     * LED 초기화
     *==============================================*/
    DDRB = 0xFF;

#if LED_ACTIVE_LOW
    PORTB = 0xFF;
#else
    PORTB = 0x00;
#endif


    /*==============================================
     * LCD 초기화
     *==============================================*/
    lcd_init();

    lcd_clear();

    lcd_gotoxy(0, 0);

    lcd_string("Buffer Count:");


    /*==============================================
     * Producer 5개 생성
     *==============================================*/
    for (i = 0; i < NUM_PRODUCERS; i++) {
        result = xTaskCreate(vProducerTask, "Producer", 80, &producer_id[i], 1, NULL);


        if (result != pdPASS) {
            /*
             * Task 생성 실패
             */
            PORTB = 0x00;
            while (1){}
        }
    }

    /*==============================================
     * Consumer 5개 생성
     *==============================================*/
    for (i = 0; i < NUM_CONSUMERS; i++) {
        result = xTaskCreate(vConsumerTask, "Consumer", 80, &consumer_id[i], 1, NULL);

        if (result != pdPASS) {
            PORTB = 0x00;

            while (1) {}
        }
    }

    /*==============================================
     * LCD Task
     *==============================================*/
    result = xTaskCreate(vLCDTask, "LCD", 80, NULL, 1, NULL);

    if (result != pdPASS) {
        PORTB = 0x00;

        while (1) {}
    }

    /*==============================================
     * Scheduler 시작
     *==============================================*/
    vTaskStartScheduler();

    /*
     * Scheduler가 정상 실행되면
     * 여기까지 오지 않음
     */
    PORTB = 0x00;

    while (1) {}

    return 0;
}