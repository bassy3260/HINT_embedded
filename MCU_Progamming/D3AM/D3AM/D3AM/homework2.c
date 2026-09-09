/*
 * homework2.c
 *
 * 3일차 오전 실습 2 — PWM 으로 밝기 조절
 *
 *  Timer3 Fast PWM 8-bit · 분주 64 -> 900 Hz  (OC3A = PE3, 보드의 D9)
 *  D9 는 Active-Low 라서 드라이버가 반전(COM3A1:0 = 11) 을 쓴다.
 *
 *  50 ms tick 마다 듀티를 5 % 씩 올리고 내려 "호흡 효과" 를 만든다.
 *  블로킹이 없으므로 그동안 다른 일도 함께 돈다.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "timer.h"
#include "pwm.h"
#include "uart.h"

int main2(void)
{
    uint8_t tev;
    uint8_t duty = 0;
    int8_t  step = 5;

    timer_init();
    pwm_init();
    uart_init();

    sei();
    uart_puts("\n=== Timer3 Fast PWM  900 Hz  (OC3A = PE3, D9) ===\n");

    while (1) {
        tev = timer_get_events();

        if (tev & TIMER_EVENT_50MS) {               /* 50 ms 마다 한 단계 */
            int16_t next = (int16_t)duty + step;    /* uint8_t 로 더하면 넘친다 */

            if (next >= 100) { next = 100; step = -5; }
            if (next <= 0)   { next = 0;   step =  5; }

            duty = (uint8_t)next;
            pwm_set_duty(duty);
        }

        if (tev & TIMER_EVENT_1S) {
            uart_puts("duty=");
            uart_put_uint16(duty);
            uart_puts("%\n");
        }
    }
}

/*
 *  확인할 것
 *   1) D9 가 0 % 에서 100 % 까지 매끄럽게 밝아지고 어두워지는가
 *   2) duty 를 int16_t 로 계산하는 이유 — uint8_t 로 하면 0 아래에서 255 가 된다
 *   3) I/O View 에서 OCR3A 가 듀티에 따라 바뀌는지 확인한다
 *   4) 분주를 64 -> 8 로 바꾸면 주파수가 얼마가 되는가 (7.2 kHz)
 */
