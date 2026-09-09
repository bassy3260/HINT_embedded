/*
 * timer.h — Timer0 기반 시간 tick (SDK ATMEGA128A, F_CPU = 14.7456 MHz)
 *
 *  14.7456 MHz 는 UART 보레이트에는 완벽하지만 1 ms 로는 나누어떨어지지 않는다.
 *  1024 분주하면 타이머 클럭이 정확히 14,400 Hz 가 되고,
 *  144 카운트(OCR0 = 143)가 정확히 10 ms 다.  → 오차 0 %
 *
 *      f_tick = 14,745,600 / (1024 x (1 + 143)) = 100 Hz  →  10 ms
 *
 *  분주만 256 으로 바꾸면 같은 OCR0 로 2.5 ms 가 된다 (FND 다중화용).
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

#define TIMER_TICK_MS       10

#define TIMER_EVENT_10MS    (1 << 0)
#define TIMER_EVENT_50MS    (1 << 1)
#define TIMER_EVENT_250MS   (1 << 2)
#define TIMER_EVENT_1S      (1 << 3)

void     timer_init(void);          /* Timer0 CTC 10 ms tick 시작 */
uint8_t  timer_get_events(void);    /* 쌓인 주기 이벤트를 읽고 지운다 */
uint32_t timer_millis(void);        /* 부팅 후 경과 시간 [ms] */

#endif /* TIMER_H_ */
