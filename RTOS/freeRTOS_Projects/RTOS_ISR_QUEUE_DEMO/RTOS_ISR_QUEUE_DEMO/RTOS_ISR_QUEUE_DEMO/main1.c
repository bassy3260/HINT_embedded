#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "lcd.h"

/*==============================================
 * 설정
 *==============================================*/

#define QUEUE_SIZE        8

#define NUM_PRODUCERS     5
#define NUM_CONSUMERS     5

#define LED_ACTIVE_LOW    1

/*
 * Producer 랜덤 대기 시간
 * 1 ~ 3초
 */
#define PRODUCER_MIN_DELAY    1000
#define PRODUCER_MAX_DELAY    3000

/*
 * Consumer 랜덤 대기 시간
 * 3 ~ 6초
 *
 * Producer보다 느리게 설정하여
 * Queue가 채워지는 모습을 쉽게 관찰
 */
#define CONSUMER_MIN_DELAY    3000
#define CONSUMER_MAX_DELAY    6000

/*==============================================
 * FreeRTOS 객체
 *==============================================*/

static QueueHandle_t xDataQueue;
static SemaphoreHandle_t xClearSemaphore;
static SemaphoreHandle_t xFillSemaphore;     /* [추가] ISR(SW3) → Fill Task 신호용 */

/*==============================================
 * Producer / Consumer ID
 *==============================================*/

static uint8_t producer_id[NUM_PRODUCERS] = {0, 1, 2, 3, 4};

static uint8_t consumer_id[NUM_CONSUMERS] = {0, 1, 2, 3, 4};

/*==============================================
 * 함수 선언
 *==============================================*/

static void vProducerTask(void *pvParameters);
static void vConsumerTask(void *pvParameters);
static void vQueueClearTask(void *pvParameters);
static void vQueueFillTask(void *pvParameters);   /* [추가] */
static void vLCDTask(void *pvParameters);

static void update_led(void);
static void sw2_interrupt_init(void);

static uint16_t random_number(uint16_t *seed);
static uint16_t random_delay(uint16_t *seed, uint16_t min, uint16_t max);

/*==============================================
 * 간단한 의사 난수 생성기
 *
 * Task마다 자신의 seed를 사용
 *==============================================*/

static uint16_t random_number(uint16_t *seed) {
    *seed = (*seed * 25173U) + 13849U;

    return *seed;
}

/*==============================================
 * min ~ max 사이 랜덤 시간 생성
 *==============================================*/

static uint16_t random_delay(uint16_t *seed, uint16_t min, uint16_t max) {
    uint16_t range;

    range = max - min + 1;

    return min + (random_number(seed) % range);
}

/*==============================================
 * LED 표시
 *
 * Queue에 들어있는 데이터 개수만큼
 * LED를 누적해서 켬
 *
 * Queue = 0
 * ○ ○ ○ ○ ○ ○ ○ ○
 *
 * Queue = 3
 * ● ● ● ○ ○ ○ ○ ○
 *
 * Queue = 8
 * ● ● ● ● ● ● ● ●
 *==============================================*/

static void update_led(void) {
    UBaseType_t count;

    uint8_t pattern;

    /*
     * 현재 Queue에 저장된 데이터 개수
     */
    count = uxQueueMessagesWaiting(xDataQueue);

    if (count == 0) { pattern = 0x00; }
    else if (count >= QUEUE_SIZE) { pattern = 0xFF; }
    else { pattern = (1U << count) - 1U; }

#if LED_ACTIVE_LOW
    /*
     * Active Low
     *
     * 0 = LED ON
     */
    PORTB = ~pattern;
#else
    /*
     * Active High
     */
    PORTB = pattern;

#endif
}

/*==============================================
 * Producer Task
 *==============================================*/

static void vProducerTask(void *pvParameters) {
    uint8_t id;
    uint8_t data = 0;

    uint16_t seed;
    uint16_t delay_time;

    /*
     * Producer 번호
     */
    id = *((uint8_t *)pvParameters);

    /*
     * Producer별로 서로 다른 seed 사용
     */
    seed = 1000U + ((uint16_t)id * 3000U);

    while (1) {
        /*----------------------------------------
         * 랜덤 대기 시간 생성
         *----------------------------------------*/

        delay_time = random_delay(&seed, PRODUCER_MIN_DELAY, PRODUCER_MAX_DELAY);

        /*----------------------------------------
         * 랜덤 시간 동안 대기
         *----------------------------------------*/

        vTaskDelay(pdMS_TO_TICKS(delay_time));

        /*----------------------------------------
         * 데이터 생성
         *----------------------------------------*/

        data++;

        if (data > 8) { data = 1; }

        /*----------------------------------------
         * Queue에 데이터 삽입
         *
         * Queue가 가득 차면
         * 빈 공간이 생길 때까지 Block
         *----------------------------------------*/

        if (xQueueSend(xDataQueue, &data, portMAX_DELAY) == pdPASS) {
            /*
             * Queue 상태를 LED에 표시
             */
            update_led();
        }
    }
}

/*==============================================
 * Consumer Task
 *==============================================*/

static void vConsumerTask(void *pvParameters){
    uint8_t id;
    uint8_t data;

    uint16_t seed;
    uint16_t delay_time;

    /*
     * Consumer 번호
     */
    id = *((uint8_t *)pvParameters);

    /*
     * Consumer별 서로 다른 seed
     *
     * Producer와도 다른 범위 사용
     */
    seed = 30000U + ((uint16_t)id * 2000U);

    while (1) {
        /*----------------------------------------
         * 랜덤 대기 시간
         *----------------------------------------*/

        delay_time = random_delay(&seed, CONSUMER_MIN_DELAY, CONSUMER_MAX_DELAY);

        /*----------------------------------------
         * 랜덤 시간 동안 대기
         *----------------------------------------*/

        vTaskDelay(pdMS_TO_TICKS(delay_time));

        /*----------------------------------------
         * Queue에서 데이터 소비
         *
         * Queue가 비어 있으면
         * 데이터가 들어올 때까지 Block
         *----------------------------------------*/

        if (xQueueReceive(xDataQueue, &data, portMAX_DELAY) == pdPASS) {
            /*
             * 이 예제에서는 data 자체는
             * 사용하지 않음
             */
            (void)data;
            /*
             * Queue 상태 LED 갱신
             */
            update_led();
        }
    }
}


/*==============================================
 * LCD Task
 *
 * Queue에 들어 있는 데이터 개수를
 * LCD에 출력
 *==============================================*/

static void vLCDTask(void *pvParameters) {
    UBaseType_t count;

    (void)pvParameters;

    while (1) {
        /*
         * 현재 Queue 데이터 개수
         */
        count = uxQueueMessagesWaiting(xDataQueue);

        /*----------------------------------------
         * 첫 번째 줄
         *----------------------------------------*/
        lcd_gotoxy(0, 0);
        lcd_string("Queue Count:    " );

        /*----------------------------------------
         * 두 번째 줄
         *----------------------------------------*/

        lcd_gotoxy(0, 1);

        lcd_string("Count = ");

        /*
         * Queue 크기가 8이므로
         * 실제 값은 0 ~ 8
         */
        if (count <= 9) {lcd_data('0' + (uint8_t)count); }
        else        { lcd_data('?'); }

        /*
         * 이전 표시 문자 제거
         */
        lcd_string("       ");

        /*
         * LCD 갱신 주기
         */
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/*==============================================
 * Queue Clear Task
 *
 * SW2를 누르면 INT4 발생
 * → ISR이 Semaphore Give
 * → 이 Task가 깨어남
 * → Queue 전체 삭제
 *==============================================*/

static void vQueueClearTask(void *pvParameters){
    (void)pvParameters;

    while (1) {
        /*----------------------------------------
         * INT4 ISR의 신호를 기다림
         *----------------------------------------*/
        xSemaphoreTake(xClearSemaphore, portMAX_DELAY );

        /*----------------------------------------
         * Queue 전체 삭제
         *----------------------------------------*/

        xQueueReset(xDataQueue );

        /*----------------------------------------
         * LED 갱신
         *
         * Queue가 비어 있으므로
         * LED 모두 OFF
         *----------------------------------------*/

        update_led();
    }
}

/*==============================================
 * [추가] Queue Fill Task
 *
 * SW3를 누르면 INT5 발생
 * → ISR이 Semaphore Give
 * → 이 Task가 깨어남
 * → Queue가 가득 찰 때까지 채움 → LED 전부 ON
 *==============================================*/

static void vQueueFillTask(void *pvParameters){
    uint8_t data = 9;

    (void)pvParameters;

    while (1) {
        /*----------------------------------------
         * INT5 ISR의 신호를 기다림
         *----------------------------------------*/
        xSemaphoreTake(xFillSemaphore, portMAX_DELAY);

        /*----------------------------------------
         * Queue가 가득 찰 때까지 삽입
         *
         * 대기 시간 0 → 꽉 차면 기다리지 않고 바로 실패
         * → 실패하는 순간 while 종료
         *----------------------------------------*/
        while (xQueueSend(xDataQueue, &data, 0) == pdPASS) {
        }

        /*----------------------------------------
         * LED 갱신 (Queue = 8 → LED 모두 ON)
         *----------------------------------------*/
        update_led();
    }
}

/*==============================================
 * SW2 / INT4 초기화
 *
 * SW2 = PE4 / INT4
 *
 * Active Low 방식
 *
 * PE4 -------- SW2 -------- GND
 *
 * 평상시      PE4 = HIGH
 * 버튼 누름   PE4 = LOW
 *
 * 따라서 Falling Edge 사용
 *==============================================*/

static void sw2_interrupt_init(void) {
    /*----------------------------------------
     * PE4 입력
     *----------------------------------------*/
    DDRE &= ~(1 << PE4);
    DDRE &= ~(1 << PE5);        /* [추가] SW3 = PE5 입력 */

    /*----------------------------------------
     * 내부 Pull-up 활성화
     *----------------------------------------*/

    PORTE |= (1 << PE4);
    PORTE |= (1 << PE5);        /* [추가] PE5 Pull-up */

    /*----------------------------------------
     * INT4 Falling Edge
     *
     * INT4 ~ INT7은 EICRB 사용
     *
     * ISC41 ISC40
     *   1     0
     *
     * Falling Edge
     *----------------------------------------*/

    EICRB |= (1 << ISC41);

    EICRB &= ~(1 << ISC40);

    EICRB |= (1 << ISC51);      /* [추가] INT5 Falling Edge (ISC51=1, ISC50=0) */
    EICRB &= ~(1 << ISC50);

    /*----------------------------------------
     * 기존 Interrupt Flag 제거
     *----------------------------------------*/

    EIFR = (1 << INTF4) | (1 << INTF5);     /* [수정] INT5 플래그도 제거 */

    /*----------------------------------------
     * INT4 Enable
     *----------------------------------------*/

    EIMSK |= (1 << INT4) | (1 << INT5);     /* [수정] INT5도 허용 */
}

/*==============================================
 * INT4 ISR
 *
 * SW2를 누르면 실행
 *
 * ISR에서는 Queue Reset을 직접 하지 않고
 * Semaphore로 Clear Task에 신호만 전달
 *==============================================*/

ISR(INT4_vect) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /*----------------------------------------
     * Clear Task에게 신호 전달
     *----------------------------------------*/

    xSemaphoreGiveFromISR(xClearSemaphore, &xHigherPriorityTaskWoken);

    /*
     * 현재 사용하는 AVR FreeRTOS Port에서
     * 지원한다면 다음을 사용할 수 있습니다.
     *
     * portYIELD_FROM_ISR(
     *     xHigherPriorityTaskWoken
     * );
     */
}


/*==============================================
 * [추가] INT5 ISR
 *
 * SW3를 누르면 실행
 * Fill Task에 신호만 전달
 *==============================================*/

ISR(INT5_vect) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(xFillSemaphore, &xHigherPriorityTaskWoken);
}


/*==============================================
 * main
 *==============================================*/

int main(void) {
    uint8_t i;

    BaseType_t result;

    /*==============================================
     * LED 초기화
     *
     * PORTB = LED 8개
     *==============================================*/

    DDRB = 0xFF;

#if LED_ACTIVE_LOW
    /*
     * Active Low
     *
     * 모두 OFF
     */
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

    lcd_string("Queue Count:");

    lcd_gotoxy(0, 1);

    lcd_string("Count = 0");

    /*==============================================
     * Queue 생성
     *
     * uint8_t 데이터 최대 8개 저장
     *==============================================*/

    xDataQueue = xQueueCreate(QUEUE_SIZE, sizeof(uint8_t));

    /*==============================================
     * ISR → Clear Task용
     * Binary Semaphore 생성
     *==============================================*/

    xClearSemaphore = xSemaphoreCreateBinary();

    xFillSemaphore = xSemaphoreCreateBinary();      /* [추가] */

    /*==============================================
     * Queue / Semaphore 생성 실패 검사
     *==============================================*/

    if ((xDataQueue == NULL) || (xClearSemaphore == NULL) || (xFillSemaphore == NULL)) {   /* [수정] */
        /*
         * Active Low
         * LED 전체 ON = 오류 표시
         */
        PORTB = 0x00;

        while (1) {}
    }

    /*==============================================
     * Producer 5개 생성
     *==============================================*/

    for (i = 0; i < NUM_PRODUCERS; i++) {
        result = xTaskCreate(vProducerTask, "Producer", 64, &producer_id[i], 1, NULL);

        /*
         * Task 생성 실패
         */
        if (result != pdPASS) {
            PORTB = 0x00;

            while (1) {}
        }
    }

    /*==============================================
     * Consumer 5개 생성
     *==============================================*/

    for (i = 0; i < NUM_CONSUMERS; i++) {
        result = xTaskCreate(vConsumerTask, "Consumer", 64, &consumer_id[i], 1, NULL);

        if (result != pdPASS) {
            PORTB = 0x00;

            while (1) {}
        }
    }

    /*==============================================
     * Queue Clear Task
     *
     * Producer / Consumer보다 높은 Priority
     *==============================================*/

    result = xTaskCreate(vQueueClearTask, "Clear", 64, NULL, 2, NULL);


    if (result != pdPASS) {
        PORTB = 0x00;

        while (1) {}
    }

    /*==============================================
     * [추가] Queue Fill Task
     *
     * Clear Task와 같은 Priority 2
     *==============================================*/

    result = xTaskCreate(vQueueFillTask, "Fill", 64, NULL, 2, NULL);

    if (result != pdPASS) {
        PORTB = 0x00;

        while (1) {}
    }

    /*==============================================
     * LCD Task 생성
     *==============================================*/

    result = xTaskCreate(vLCDTask, "LCD", 80, NULL, 1, NULL);

    if (result != pdPASS) {
        PORTB = 0x00;

        while (1) {}
    }


    /*==============================================
     * SW2 / INT4 초기화
     *==============================================*/
    sw2_interrupt_init();

    /*==============================================
     * FreeRTOS Scheduler 시작
     *==============================================*/

    vTaskStartScheduler();

    /*
     * 정상적으로 Scheduler가 시작되면
     * 여기까지 도달하지 않음
     */

    PORTB = 0x00;

    while (1) {}

    return 0;
}