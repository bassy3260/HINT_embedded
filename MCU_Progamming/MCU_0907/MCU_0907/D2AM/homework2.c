/*
 * homework2.c
 *
 * 2일차 오전 실습 2 — 버튼 네 개를 동시에 처리
 *
 *  벡터 네 개(INT4~INT7)를 모두 쓰고, main 은 btn_get_events() 한 곳에서 받는다.
 *  보드 : SW2~SW5 = PE4~PE7 = INT4~INT7 (Active-Low, 하강 에지)
 *
 *  버튼 하나가 LED 하나를 토글한다. 두 버튼을 거의 동시에 누르면
 *  한 번의 btn_get_events() 로 두 비트가 함께 올라와 LED 두 개가 함께 바뀐다.
 *
 *  ISR 은 여전히 "플래그 한 줄" 이고, 판단과 출력은 전부 main 이 한다.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "led.h"
#include "btn.h"

int main3(void)
{
    led_init();
    btn_init();
    btn_int_enable();

    sei();                        /* 전역 허용은 마지막에 한 번만 */

    while (1) {
        uint8_t ev = btn_get_events();      /* 쌓인 이벤트를 한 번에 */

        if (ev & BTN_EVENT_SW2) led_toggle(0);
        if (ev & BTN_EVENT_SW3) led_toggle(1);
        if (ev & BTN_EVENT_SW4) led_toggle(2);
        if (ev & BTN_EVENT_SW5) led_toggle(3);
    }
}

/*  확인할 것
 *    · 버튼 네 개가 각각 D1~D4 를 따로 토글하는가
 *    · 두 버튼을 거의 동시에 눌러도 둘 다 반영되는가 (ev 에 두 비트가 함께)
 *    · 한 번 눌렀는데 두 번 토글되지 않는가 (채터링 — 3.8 절)
 *
 *  막히면 여기부터
 *    · sei() 를 불렀는가 · EIMSK 를 켰는가 (btn_int_enable)
 *    · EICRB 를 하강 에지로 맞췄는가
 *    · btn.c 의 event 에 volatile 이 붙어 있는가
 */
