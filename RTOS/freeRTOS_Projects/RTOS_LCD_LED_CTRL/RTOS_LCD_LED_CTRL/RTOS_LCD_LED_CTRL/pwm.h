/*
 * pwm.h — Timer3 Fast PWM (SDK ATMEGA128A : OC3A = PE3) — FAN 모듈용
 *
 *  FAN 모듈 연결
 *      신호(IN/PWM/S) -> PE3 (OC3A)
 *      VCC            -> +5V
 *      GND            -> GND
 *
 *  FAN 모듈은 Active-High 이므로 **Non-Inverting 모드**를 쓴다.
 *      COM3A1:COM3A0 = 10   BOTTOM 에서 HIGH, 비교일치에서 LOW
 *      -> OCR3A ↑  ->  HIGH 구간 ↑  ->  FAN 속도 ↑
 *
 *  같은 핀의 D9 LED(Active-Low)는 FAN 과 반대로 밝기가 변한다.
 */

#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>

void pwm_init(void);                  /* 8비트 Fast PWM · 900 Hz · 정지 상태로 시작 */
void pwm_set_duty(uint8_t percent);   /* 0 ~ 100 [%] — 클수록 빠르다 */
void pwm_set_raw(uint8_t value);      /* 0 ~ 255 — 계산 없이 바로 */

#endif /* PWM_H_ */