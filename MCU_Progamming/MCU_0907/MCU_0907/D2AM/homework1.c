/*
 * homework1.c
 *
 * 2일차 오전 실습 1 — 폴링과 인터럽트 비교
 *
 *  2일차 오전 : INT4 ~ INT7 · ISR · 이벤트 플래그 패턴
 *  보드       : SW2 = PE4 = INT4  (Active-Low, 하강 에지)
 *
 *  LED 가 한 칸씩 흐르는 동안 SW2 를 짧게 눌러 본다.
 *  폴링이라면 _delay_ms 안에 있는 동안 눌림을 놓치지만,
 *  인터럽트는 ISR 이 플래그를 설정해 두므로 다음 바퀴에 반드시 반영된다.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include <util/delay.h>
#include "led.h"
#include "btn.h"

int main4(void)
{
    uint8_t pos     = 0;
    uint8_t running = 1;

    led_init();
    btn_init();
    btn_int_enable();

    sei();                              /* 초기화가 모두 끝난 뒤 딱 한 번 */

    while (1) {
        if (btn_get_events() & BTN_EVENT_SW2)   /* 쌓인 눌림을 받는다 */
            running = !running;                 /* 멈춤 / 재개 */

        if (running) {
            led_write((uint8_t)(1 << pos));
            if (++pos >= 8)
                pos = 0;
        }

        _delay_ms(150);                 /* 이 안에서 눌러도 놓치지 않는다 */
    }
}

/*
 *  관찰할 것
 *   1) _delay_ms 가 도는 중에 짧게 눌러도 다음 바퀴에 멈춘다
 *   2) ISR(INT4_vect) 안에 breakpoint 를 걸어 실제로 불리는지 확인한다
 *   3) 한 번 눌렀는데 두 번 처리되는 일이 있다 — 채터링이다
 *      해결은 3일차 Timer 로 시간 기준을 만든 뒤에
 *   4) btn.c 의 btn_event 에서 volatile 을 빼고 -O2 로 빌드해 보라
 */
