/*
 * homework1.c
 *
 * 3일차 오후 실습 1 — 조도 센서 읽기
 *
 *  3일차 오후 : ADC 구조 · 주기 샘플링 · 변환 완료 인터럽트
 *  보드       : CDS 조도센서 = PF1(ADC1)
 *               AREF 가 AVCC 에 묶여 있으므로 REFS1:0 = 01 (AVCC)
 *  CDS 회로 : +5V - CDS - PF1 - 10kΩ - GND
 *             밝으면 CDS 저항이 낮아져 PF1 전압이 올라간다 -> 값이 커진다
 *
 *  10 ms tick 으로 50 ms 마다 변환을 요청하고, 결과는 인터럽트로 받는다.
 *  변환이 끝날 때까지 기다리지 않으므로 그동안 다른 일도 함께 돈다.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "led.h"
#include "timer.h"
#include "adc.h"
#include "uart.h"

int main1(void)
{
    uint8_t  tev, aev;
    uint16_t val[2] = {0, 0};

    led_init();
    timer_init();
    adc_init();
    uart_init();

    sei();
    uart_puts("\n=== ADC1 (CDS) ===\n");

    while (1) {
        tev = timer_get_events();

        if (tev & TIMER_EVENT_50MS)             /* 20 Hz 로 변환을 요청한다 */
            adc_start(ADC_CH_CDS);

        aev = adc_get_event(val);               /* 끝난 변환이 있으면 값을 받는다 */
        if (aev & ADC_EVENT_CDS) {
            /* 0 ~ 1023 을 LED 8칸 막대로 — 밝으면 막대가 길어진다 */
            led_write((uint8_t)((1u << (val[ADC_CH_CDS] >> 7)) - 1));
        }

        if (tev & TIMER_EVENT_250MS) {          /* 4 Hz 로 로그 */
            uart_puts("CDS=");
            uart_put_uint16(val[ADC_CH_CDS]);
            uart_puts("\n");
        }
    }
}

/*
 *  확인할 것
 *   1) 센서를 손으로 가리면 값이 어느 쪽으로 움직이는가 (작아진다)
 *   2) LED 막대가 조도에 따라 길어지고 짧아지는가
 *   3) 값이 조금씩 흔들린다 — 다음 실습의 히스테리시스가 필요한 이유다
 *   4) adc_read_blocking() 으로 바꾸면 로그 주기가 흔들리는가
 */
