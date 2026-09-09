/*
 * homework1.c
 *
 * 3일차 오전 실습 1 — delay 를 tick 으로
 *
 *  3일차 오전 : Timer0 CTC · 10 ms tick · 논블로킹 Super Loop
 *  보드       : 14.7456 MHz / 1024 = 14,400 Hz, OCR0 = 143 -> 정확히 10 ms
 *
 *  _delay_ms 가 한 줄도 없다. 하나의 tick 에서 250 ms 와 1 s 를 함께 만든다.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "led.h"
#include "timer.h"
#include "uart.h"

int main1(void)
{
    uint8_t tev;
    uint8_t pos = 0;

    led_init();
    timer_init();
    uart_init();

    sei();
    uart_puts("\n=== Timer0 CTC tick (10 ms base) ===\n");

    while (1) {
        tev = timer_get_events();

        if (tev & TIMER_EVENT_250MS) {      /* 250 ms : LED 한 칸 이동 */
            led_write((uint8_t)(1 << pos));
            if (++pos >= 8)
                pos = 0;
        }

        if (tev & TIMER_EVENT_1S) {         /* 1 초 : tick 이 정확한지 확인 */
            uart_put_uint16((uint16_t)timer_millis());
            uart_puts(" ms\n");
        }
    }
}

/*
 *  확인할 것
 *   1) 1초 로그의 ms 값이 1000 씩 정확히 늘어나는가 (CTC 라서 오차가 없다)
 *   2) LED 는 250 ms, 로그는 1 s — tick 하나로 두 주기가 함께 돈다
 *   3) Normal 모드 + TCNT0 = 112 재적재 방식으로 바꾸면 오차가 어떻게 달라지는가
 *   4) 분주를 1024 -> 256 으로 바꾸면 tick 은 몇 ms 가 되는가 (2.5 ms)
 */
