/*
 * homework3.c
 *
 * 3일차 오전 실습 3 — 부저를 길이로 구분한다
 *
 *  이 보드의 부저는 수동형이다. 전압을 걸어 두는 것만으로는 울지 않아서
 *  드라이버가 Timer1 CTC 로 1,024 Hz 구형파를 만든다 (buzzer.c 참고).
 *  app 이 정하는 것은 "길이" 뿐이다.
 *
 *      buzzer_beep(ms)  로 시작하고,
 *      10 ms tick 마다 buzzer_tick() 을 불러 주면 시간이 다 됐을 때 스스로 꺼진다.
 *
 *  블로킹하지 않으므로 비프가 울리는 동안에도 LED 는 계속 흐른다.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "led.h"
#include "btn.h"
#include "timer.h"
#include "buzzer.h"

int main3(void)
{
    uint8_t pos = 0;

    led_init();
    btn_init();
    btn_int_enable();
    buzzer_init();
    timer_init();

    sei();

    while (1) {
        uint8_t bev = btn_get_events();
        uint8_t tev = timer_get_events();

        if (bev & BTN_EVENT_SW2)  buzzer_beep(50);    /* 확인 — 짧게 */
        if (bev & BTN_EVENT_SW3)  buzzer_beep(200);   /* 경고 — 중간 */
        if (bev & BTN_EVENT_SW4)  buzzer_beep(500);   /* 오류 — 길게 */

        if (tev & TIMER_EVENT_10MS)
            buzzer_tick();                            /* 남은 길이를 센다 */

        if (tev & TIMER_EVENT_250MS) {   /* 비프와 무관하게 LED 는 계속 흐른다 */
            led_write((uint8_t)(1 << pos));
            if (++pos >= 8)
                pos = 0;
        }
    }
}

/*  확인할 것
 *    · 세 버튼의 소리 길이가 다른가 (50 / 200 / 500 ms)
 *    · 비프가 울리는 동안에도 LED 가 멈추지 않는가 — 이것이 논블로킹이다
 *    · buzzer_beep_blocking() 으로 바꿔 보고 LED 흐름이 멈추는 것을 확인한다
 *
 *  음정을 바꾸려면
 *    · buzzer.c 의 OCR1A 를 바꾼다 (지금은 899 -> 1,024 Hz)
 *    · OCR1A = F_CPU / (2 x 분주 x 주파수) - 1
 *      440 Hz(라) : 14,745,600 / (2 x 8 x 440) - 1 = 2,094
 */
