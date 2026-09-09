/*
 * pwm.h — Timer3 Fast PWM (SDK ATMEGA128A : OC3A = PE3)
 *
 *  보드의 D9 LED 가 PE3 에 Active-Low 로 달려 있다 (PE3 - LED - 10kΩ - +5V).
 *  그래서 OCR3A 를 키울수록 밝아지게 하려면 **Inverting 모드**를 쓴다.
 *      COM3A1:COM3A0 = 11   비교일치에서 HIGH, TOP 에서 LOW
 *      -> OCR3A ↑  ->  LOW 구간 ↑  ->  LED ON 시간 ↑
 */

#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>

void pwm_init(void);                  /* 8비트 Fast PWM · 900 Hz */
void pwm_set_duty(uint8_t percent);   /* 0 ~ 100 [%] — 클수록 밝다 */
void pwm_set_raw(uint8_t value);      /* 0 ~ 255 — 계산 없이 바로 */

#endif /* PWM_H_ */
