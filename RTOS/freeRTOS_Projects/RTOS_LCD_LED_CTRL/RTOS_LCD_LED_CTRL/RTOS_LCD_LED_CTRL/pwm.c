/*
 * pwm.c — Timer3 8비트 Fast PWM (Mode 5) — FAN 모듈용 (Non-Inverting)
 *
 *  모드 선택 (ATmega128 Timer3)
 *      WGM33:WGM30 = 0101  ->  Fast PWM, 8-bit (TOP = 0x00FF)
 *      TCCR3A : WGM31=0, WGM30=1
 *      TCCR3B : WGM33=0, WGM32=1
 *
 *  출력 방식 — FAN 모듈은 Active-High 이므로 Non-Inverting 을 쓴다
 *      COM3A1:COM3A0 = 10  ->  BOTTOM 에서 HIGH, 비교일치에서 LOW
 *      OCR3A 가 클수록 HIGH(FAN 구동) 구간이 길어진다
 *
 *  주의 : 같은 핀(PE3)에 달린 D9 LED 는 Active-Low 라서
 *         FAN 이 빠를수록 D9 는 어두워진다 (반대로 보인다)
 *
 *  주파수
 *      f_pwm = F_CPU / (분주비 x (1 + TOP))
 *            = 14,745,600 / (64 x 256)  =  900 Hz     (주기 1.111 ms)
 */
#include "pwm.h"

#define PWM_TOP  255

void pwm_init(void)
{
    PORTE &= ~(1 << PWM_BIT);           /* 먼저 LOW(정지) 로 만들고 */
    PWM_DDR |= (1 << PWM_BIT);          /* 그 다음 출력으로 — 순간 회전 방지 */

    OCR3A = 0;                          /* 듀티 0 으로 시작 */

    TCCR3A = (1 << WGM30);                   /* Fast PWM 8-bit (출력은 아직 분리) */
    TCCR3B = (1 << WGM32)
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
        /* Non-Inverting 에서 OCR3A = 0 이면 한 클럭짜리 HIGH 가 남는다.
           완전히 멈추려면 비교 출력을 떼고 핀을 직접 LOW(정지)로 만든다. */
		// Timer3 설정
        TCCR3A &= ~((1 << COM3A1) | (1 << COM3A0));
        PORTE  &= ~(1 << PWM_BIT);
    } else {
        OCR3A = value;
        TCCR3A = (uint8_t)((TCCR3A & ~(1 << COM3A0)) | (1 << COM3A1));  /* 10 = Non-Inverting */
    }
}