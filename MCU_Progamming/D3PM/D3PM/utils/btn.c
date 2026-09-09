/*
 * btn.c — 스위치 드라이버
 *
 *  회로 : +5V ─ 10kΩ ─ (PEn) ─ 택트스위치 ─ GND,  PEn 에 0.1uF 병렬
 *         누르지 않음 -> 1,  누름 -> 0   (Active-Low)
 *         0.1uF 이 잡음을 조금 눌러 주지만 채터링을 없애지는 못한다.
 *
 *  PE4~PE7 은 INT4~INT7 이므로 트리거 설정은 EICRB 를 쓴다.
 *  (INT0~INT3 은 EICRA — 이 보드에서는 PD0·PD1 이 연결되어 있지 않다)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "board.h"
#include "btn.h"

/* ISR 이 쓰고 main 이 읽는다 — volatile 필수 */
static volatile uint8_t btn_event = 0;

ISR(INT4_vect) { btn_event |= BTN_EVENT_SW2; }   /* SW2 */
ISR(INT5_vect) { btn_event |= BTN_EVENT_SW3; }   /* SW3 */
ISR(INT6_vect) { btn_event |= BTN_EVENT_SW4; }   /* SW4 */
ISR(INT7_vect) { btn_event |= BTN_EVENT_SW5; }   /* SW5 */

void btn_init(void)
{
    BTN_DDR  &= ~BTN_MASK;      /* PE4 ~ PE7 을 입력으로 */
    BTN_PORT &= ~BTN_MASK;      /* 내부 풀업은 끈다 — 보드에 외부 풀업이 있다 */
}

uint8_t btn_is_pressed(uint8_t n)
{
    /* n = 2~5  ->  PE4~PE7 */
    uint8_t bit = (uint8_t)(n - BTN_NUM_BASE + BTN_PIN_BASE);
    return (BTN_PIN & (1 << bit)) == 0;      /* 눌리면 0 이므로 뒤집는다 */
}

uint8_t btn_get_edge(uint8_t n)
{
    static uint8_t prev[BTN_COUNT] = {0, 0, 0, 0};
    uint8_t idx  = (uint8_t)(n - BTN_NUM_BASE);
    uint8_t curr = btn_is_pressed(n);
    uint8_t edge = (prev[idx] == 0) && (curr == 1);

    prev[idx] = curr;
    return edge;
}

void btn_int_enable(void)
{
    /* ① 트리거 조건 — INT4~INT7 을 하강 에지로 (ISCn1:ISCn0 = 10) */
    EICRB &= ~((1 << ISC40) | (1 << ISC50) | (1 << ISC60) | (1 << ISC70));
    EICRB |=  ((1 << ISC41) | (1 << ISC51) | (1 << ISC61) | (1 << ISC71));

    /* ② 설정 중에 생긴 묵은 요청을 지운다 — 플래그는 1 을 써서 지운다 */
    EIFR  = (1 << INTF4) | (1 << INTF5) | (1 << INTF6) | (1 << INTF7);

    /* ③ 개별 허용 */
    EIMSK |= (1 << INT4) | (1 << INT5) | (1 << INT6) | (1 << INT7);

    /* 전역 허용 sei() 는 모든 드라이버 초기화가 끝난 뒤 main 에서 한 번만 */
}

uint8_t btn_get_events(void)
{
    uint8_t sreg = SREG;        /* 현재 인터럽트 허용 상태를 기억 */
    uint8_t ev;

    cli();                      /* 읽고 지우는 사이에 끼어들지 못하게 */
    ev = btn_event;
    btn_event = 0;
    SREG = sreg;                /* 원래 상태로 복원 (sei() 가 아니다) */

    return ev;
}
