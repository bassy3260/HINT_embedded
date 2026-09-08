/*
 * btn.h — 스위치 드라이버 (SDK ATMEGA128A)
 *
 *      SW2 → PE4 (INT4)      SW4 → PE6 (INT6)
 *      SW3 → PE5 (INT5)      SW5 → PE7 (INT7)
 *
 *  보드에 10kΩ 풀업 + 0.1uF 이 달려 있어 누르지 않으면 1, 누르면 0 이다.
 *  드라이버가 반전해서 "눌렸다 = 1" 로 돌려준다.
 *
 *  app 은 보드에 인쇄된 이름(SW2~SW5)을 그대로 쓴다.
 */

#ifndef BTN_H_
#define BTN_H_

#include <stdint.h>

/* app 이 쓰는 버튼 번호 — 보드 실크와 같다 */
#define BTN_SW2         2
#define BTN_SW3         3
#define BTN_SW4         4
#define BTN_SW5         5

#define BTN_EVENT_SW2   (1 << 0)      /* PE4 / INT4 */
#define BTN_EVENT_SW3   (1 << 1)      /* PE5 / INT5 */
#define BTN_EVENT_SW4   (1 << 2)      /* PE6 / INT6 */
#define BTN_EVENT_SW5   (1 << 3)      /* PE7 / INT7 */

void    btn_init(void);                 /* 입력 설정만 — 인터럽트는 아직 */
uint8_t btn_is_pressed(uint8_t n);      /* n : 2 ~ 5, 눌려 있으면 1 */
uint8_t btn_get_edge(uint8_t n);        /* 방금 눌린 순간에만 1 (폴링용) */

void    btn_int_enable(void);           /* INT4 ~ INT7 하강 에지 허용 */
uint8_t btn_get_events(void);           /* 쌓인 이벤트를 읽고 지운다 */

#endif /* BTN_H_ */
