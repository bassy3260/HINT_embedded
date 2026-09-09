/*
 * pwm.c — Timer3 8비트 Fast PWM (Mode 5)
 *
 *  모드 선택 (ATmega128 Timer3)
 *      WGM33:WGM30 = 0101  ->  Fast PWM, 8-bit (TOP = 0x00FF)
 *      TCCR3A : WGM31=0, WGM30=1
 *      TCCR3B : WGM33=0, WGM32=1
 *
 *  출력 방식 — 이 보드는 D9 가 Active-Low 라 Inverting 을 쓴다
 *      COM3A1:COM3A0 = 11  ->  비교일치에서 HIGH, TOP 에서 LOW
 *      OCR3A 가 클수록 LOW(점등) 구간이 길어진다
 *
 *  주파수
 *      f_pwm = F_CPU / (분주비 x (1 + TOP))
 *            = 14,745,600 / (64 x 256)  =  900 Hz     (주기 1.111 ms)
 *      사람 눈에 깜빡임이 보이지 않는다 (100 Hz 이상이면 충분)
 */
#include <avr/io.h>
#include "board.h"
#include "pwm.h"

#define PWM_TOP  255

void pwm_init(void)
{
    PWM_DDR |= (1 << PWM_BIT);          /* OC3A = PE3 를 출력으로 */

    OCR3A = 0;                          /* 듀티 0 으로 시작 */

    TCCR3A = (1 << COM3A1) | (1 << COM3A0)   /* Inverting 출력 */
           | (1 << WGM30);                   /* ┐ Fast PWM 8-bit */
    TCCR3B = (1 << WGM32)                    /* ┘ */
           | (1 << CS31) | (1 << CS30);      /* 분주 64 */
}

void pwm_set_duty(uint8_t percent)
{
    if (percent > 100)
        percent = 100;

    /* 0 ~ 100 을 0 ~ 255 로. 32비트로 계산해야 중간에서 넘치지 않는다 */
    pwm_set_raw((uint8_t)(((uint32_t)percent * PWM_TOP) / 100));
}

void pwm_set_raw(uint8_t value)
{
    if (value == 0) {
        /* Inverting 모드에서 OCR3A = 0 이면 한 클럭짜리 점등이 남는다.
           완전히 끄려면 비교 출력을 떼고 핀을 직접 HIGH(소등)로 만든다. */
        TCCR3A &= ~((1 << COM3A1) | (1 << COM3A0));
        PORTE  |= (1 << PWM_BIT);
    } else {
        TCCR3A |= (1 << COM3A1) | (1 << COM3A0);
        OCR3A = value;
    }
}
