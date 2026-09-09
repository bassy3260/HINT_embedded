/*
 * buzzer.h — 부저 드라이버 (SDK ATMEGA128A : PG3, Active-High)
 *
 *  회로 : +5V ─ 1kΩ ─ BUZ ─ PG3        PG3 = 1 일 때 전류가 흐른다
 *
 *  이 보드의 부저는 **수동형(passive)** 이다. 안에 발진 회로가 없어서
 *  핀에 전압을 걸어 두는 것만으로는 울지 않는다 — 구형파를 만들어 줘야 한다.
 *  (HW 트랙 예제도 PG3 를 500 us 마다 토글해 소리를 낸다)
 *
 *  톤은 Timer1 이 만든다. Timer0 은 10 ms tick, Timer3 은 PWM 으로 쓰이므로
 *  비어 있는 Timer1 을 쓴다. 자세한 계산은 buzzer.c 위쪽에 있다.
 *  OC1A~OC1C 는 PB5~PB7 이라 부저(PG3)에 닿지 않아서, 하드웨어 출력이 아니라
 *  ISR 에서 직접 토글한다.
 *
 *  참고 : PG3 는 TOSC2 겸용이다. ASSR 의 AS0 을 켜면(비동기 Timer0 — RTC 용)
 *         이 핀은 32.768 kHz 크리스털 전용이 되어 I/O 로 쓸 수 없다.
 */

#ifndef BUZZER_H_
#define BUZZER_H_

#include <stdint.h>

void buzzer_init(void);
void buzzer_on(void);           /* 톤 시작 — Timer1 을 돌린다 */
void buzzer_off(void);          /* 톤 정지 — Timer1 을 멈추고 핀을 LOW 로 */

/* 아래 두 개는 한 쌍이다.
   buzzer_beep(ms) 로 시작하고, 10 ms tick 마다 buzzer_tick() 을 부르면
   시간이 다 됐을 때 드라이버가 스스로 끈다. */
void buzzer_beep(uint16_t ms);
void buzzer_tick(void);

void buzzer_beep_blocking(uint16_t ms);   /* 블로킹 — 타이머 배우기 전 실습용 */

#endif /* BUZZER_H_ */
