/*
 * homework2.c
 *
 * 3일차 오후 통합 프로젝트 — Auto Light ECU
 *
 *  3일차 오후 : 드라이버 스택 + 상태 기계 조립
 *  보드       : SDK ATMEGA128A 2020
 *      입력  CDS = ADC1(PF1) · VR3 = ADC0(PF0) · SW2~SW5 = PE4~PE7(INT4~7)
 *      시간  Timer0 CTC 10 ms tick
 *      출력  PWM = OC3A(PE3, D9) · TEXT LCD(PG0~2 + PORTA) · UART1(USB)
 *            LED = PORTB (상태 막대)
 *  요구사항
 *    · AUTO   : 조도에 따라 자동으로 밝기 결정 (어두우면 밝게)
 *    · MANUAL : 가변저항 VR3 로 밝기를 직접 조절
 *    · SW2 로 모드 전환, SW3 / SW4 로 미세 조정 (MANUAL 보조)
 *    · 경계에서 떨리지 않도록 히스테리시스 적용
 *    · 상태 · 조도 · 듀티를 LCD 와 UART 로 보여 준다
 *  구조
 *    ISR      : 이벤트 플래그만 설정한다 (버튼 · 타이머 · ADC)
 *    main     : 이벤트를 모아 상태를 갱신하고 출력을 만든다
 *    블로킹 delay 없음 (lcd_init 의 초기화 delay 만 예외)
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include <stdint.h>

#include "utils/led.h"
#include "utils/btn.h"
#include "utils/timer.h"
#include "utils/adc.h"
#include "utils/pwm.h"
#include "utils/clcd.h"
#include "utils/uart.h"

/* ------------------------------------------------------------ 상태 정의 */
typedef enum {
    ST_AUTO = 0,
    ST_MANUAL
} state_t;

/* 조도 임계값 — 히스테리시스 (경계에서 떨리는 것을 막는다)
   실습실 조명에 따라 달라지므로 반드시 raw 값을 먼저 로그로 확인하고 정한다 */
#define DARK_ENTER   380        /* 이보다 어두워지면 "어둡다" 로 */
#define DARK_EXIT    460        /* 이보다 밝아져야 "밝다" 로 되돌아온다 */

#define AVG_N        8

static uint16_t avg_buf[AVG_N];
static uint8_t  avg_idx;
static uint32_t avg_sum;

static uint16_t moving_average(uint16_t v)
{
    avg_sum -= avg_buf[avg_idx];
    avg_buf[avg_idx] = v;
    avg_sum += v;
    if (++avg_idx >= AVG_N)
        avg_idx = 0;
    return (uint16_t)(avg_sum / AVG_N);
}

/* ------------------------------------------------------------ 상태 전이 */
static state_t next_state(state_t s, uint8_t bev)
{
    if (bev & BTN_EVENT_SW2)
        return (s == ST_AUTO) ? ST_MANUAL : ST_AUTO;
    return s;
}

/* ------------------------------------------------------------ 출력 계산 */
static uint8_t auto_duty(uint16_t lux, uint8_t prev)
{
    static uint8_t dark = 0;

    if (!dark && lux < DARK_ENTER) dark = 1;
    if ( dark && lux > DARK_EXIT)  dark = 0;

    if (!dark)              return 0;       /* 밝으면 소등 */
    if (lux >= DARK_ENTER)  return prev;    /* 중간 구간은 직전 값 유지 */

    /* 어두울수록 밝게 : lux 0 -> 100 %,  DARK_ENTER -> 20 % */
    return (uint8_t)(100 - ((uint32_t)lux * 80) / DARK_ENTER);
}

static uint8_t manual_duty(uint16_t vr, uint8_t bev, uint8_t *trim)
{
    int16_t d;

    if (bev & BTN_EVENT_SW3) { if (*trim < 20)  *trim += 5; }
    if (bev & BTN_EVENT_SW4) { if (*trim >= 5)  *trim -= 5; }

    d = (int16_t)(((uint32_t)vr * 100) / 1023) + (int16_t)*trim - 10;
    if (d < 0)   d = 0;
    if (d > 100) d = 100;

    return (uint8_t)d;
}

/* LCD 표시 */
/* ------------------------------------------------------------ 표시 */
static void show_lcd(state_t s, uint16_t lux, uint8_t duty)
{
    lcd_goto(0, 0);
    lcd_puts("MODE: SEOYEONG");
   // lcd_puts(s == ST_AUTO ? "AUTO   " : "MANUAL ");

    lcd_goto(1, 0);
    lcd_puts("L:");   lcd_put_uint16(lux, 4);
    lcd_puts(" D:");  lcd_put_uint16(duty, 3);
    lcd_puts("% ");
}

static void show_uart(state_t s, uint16_t raw, uint16_t lux,
                      uint16_t vr, uint8_t duty)
{
    uart_puts("MODE=");
    uart_puts(s == ST_AUTO ? "AUTO  " : "MANUAL");
    uart_puts(" RAW=");   uart_put_uint16(raw);
    uart_puts(" LUX=");   uart_put_uint16(lux);
    uart_puts(" VR=");    uart_put_uint16(vr);
    uart_puts(" DUTY=");  uart_put_uint16(duty);
    uart_puts("%\n");
}

/* ------------------------------------------------------------ main */
int main(void)
{
    state_t  state = ST_AUTO, prev_state = ST_MANUAL;
    uint8_t  bev, tev, aev;
    uint16_t val[2] = {0, 0};
    uint16_t lux = 0;
    uint8_t  duty = 0, trim = 10;
    uint8_t  req_vr = 0;
    uint8_t  bar;

    /* --- 드라이버 초기화 --- */
    led_init();
    btn_init();
    btn_int_enable();
    timer_init();
    adc_init();
    pwm_init();
    lcd_init();
    uart_init();

    sei();                          /* 여기서 딱 한 번 */

    uart_puts("\n=== Auto Light ECU  (SDK ATmega128A) ===\n");
    lcd_clear();

    while (1) {
        /* ---------- ① 이벤트 수집 ---------- */
        bev = btn_get_events();
        tev = timer_get_events();

        if (tev & TIMER_EVENT_50MS)   adc_start(ADC_CH_CDS);   /* 20 Hz */
        if (tev & TIMER_EVENT_250MS)  req_vr = 1;              /* 4 Hz  */
        if (req_vr && adc_start(ADC_CH_VR))
            req_vr = 0;

        aev = adc_get_event(val);
        if (aev & ADC_EVENT_CDS)
            lux = moving_average(val[ADC_CH_CDS]);

        /* ---------- ② 상태 전이 ---------- */
        state = next_state(state, bev);

        /* ---------- ③ 출력 계산 ---------- */
        if (state == ST_AUTO)
            duty = auto_duty(lux, duty);
        else
            duty = manual_duty(val[ADC_CH_VR], bev, &trim);

        pwm_set_duty(duty);

        /* LED 로 조도 막대를 보여 준다 */
        bar = (uint8_t)((lux >> 7) & 0x07);
        led_write((uint8_t)((1u << (bar + 1)) - 1));

        /* ---------- ④ 표시 ---------- */
        if (state != prev_state) {
            lcd_clear();
            prev_state = state;
        }
        if (tev & TIMER_EVENT_250MS)
            show_lcd(state, lux, duty);
        if (tev & TIMER_EVENT_1S)
            show_uart(state, val[ADC_CH_CDS], lux, val[ADC_CH_VR], duty);
    }
}

/*
 *  검증 항목
 *   1) 센서를 가리면 D9 가 밝아지고 손을 떼면 어두워지는가
 *   2) 경계 조도에서 깜빡거리지 않는가 (히스테리시스가 듣는가)
 *   3) SW2 로 MANUAL 로 바꾸면 조도와 무관하게 VR3 로만 바뀌는가
 *   4) 블로킹 delay 가 한 곳도 남아 있지 않은가 (lcd_init 은 예외)
 *   5) ISR 은 전부 플래그만 설정하고 끝나는가
 *   6) main 에 PORTB / ADMUX 같은 레지스터 이름이 남아 있지 않은가
 */
