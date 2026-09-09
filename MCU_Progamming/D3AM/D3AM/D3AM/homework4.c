/*
 * homework4.c
 *
 * 3일차 오전 실습 4 — 어제 코드에서 _delay 를 전부 없애기
 *
 *  기능은 1일차 오후와 똑같다 : 버튼으로 모드를 바꾸고 LED 패턴을 돌린다.
 *  달라진 것은 구조뿐 — 기다리지 않고, tick 이벤트로 진행한다.
 *
 *      1일차 : while(1) { pattern(); _delay_ms(200); }
 *      3일차 : while(1) { tev = timer_get_events(); if (tev & 250MS) step(); }
 *
 *  코드에서 _delay 를 검색해 한 건도 남지 않으면 성공이다.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "led.h"
#include "btn.h"
#include "timer.h"

/* 한 번 부를 때마다 딱 한 단계만 진행한다 — 안에서 기다리지 않는다 */
static void pattern_step(uint8_t mode)
{
    static uint8_t pos;

    if (mode == 0)
        led_write((uint8_t)(1 << pos));                 /* 순차 점등 */
    else
        led_write((uint8_t)((pos & 1) ? 0x55 : 0xAA));  /* 교차 */

    if (++pos >= 8)
        pos = 0;
}

int main4(void)
{
    uint8_t mode = 0;

    led_init();
    btn_init();
    btn_int_enable();
    timer_init();

    sei();

    while (1) {
        uint8_t bev = btn_get_events();
        uint8_t tev = timer_get_events();

        /* ① 이벤트 수집 → ② 판단 → ③ 출력 : 순서를 지킨다 */

        if (bev & BTN_EVENT_SW2)            /* 모드 전환 — 즉시 먹는다 */
            mode = !mode;

        if (tev & TIMER_EVENT_250MS)        /* 패턴 한 단계 */
            pattern_step(mode);
    }
}

/*  1일차 코드와 비교해서 확인할 것
 *    · 패턴이 도는 중에도 버튼이 즉시 먹는가 (1일차에는 한 바퀴 기다려야 했다)
 *    · pattern_step 안에 _delay 가 한 줄도 없는가
 *
 *  바뀐 것 정리
 *    _delay_ms(500); led_toggle();   →  if (tev & TIMER_EVENT_250MS) step();
 *    pattern() 안의 _delay_ms        →  단계를 상태(static pos)로 쪼갠다
 */
