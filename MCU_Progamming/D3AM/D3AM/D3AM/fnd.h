/*
 * fnd.h — 4자리 FND 드라이버 (SDK ATMEGA128A)
 *
 *      세그먼트 A~DP = PC0 ~ PC7   (공통 애노드 CA -> 0 이 점등, 330Ω)
 *      자리 COM1~COM4 = PD4 ~ PD7  (1 이 그 자리 선택)
 *
 *  한 순간에 한 자리만 켜고 빠르게 돌린다 (다중화).
 *  fnd_scan() 을 2.5 ms 마다 부르면 4자리 갱신율이 100 Hz 가 되어
 *  잔상으로 네 자리가 함께 켜져 보인다.
 *  (Timer0 을 1024 대신 256 분주로 두면 같은 OCR0 로 2.5 ms tick 이 된다)
 */

#ifndef FND_H_
#define FND_H_

#include <stdint.h>

void fnd_init(void);
void fnd_set_number(uint16_t value);   /* 0 ~ 9999 */
void fnd_clear(void);
void fnd_scan(void);                   /* 주기적으로 호출 (Timer tick 권장) */
void fnd_digit(uint8_t pos, uint8_t num);  /* 한 자리만 직접 켠다 (pos 0~3) */

#endif /* FND_H_ */
