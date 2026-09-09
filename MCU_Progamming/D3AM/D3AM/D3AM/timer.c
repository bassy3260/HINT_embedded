/*
 * timer.c — Timer0 CTC 로 10 ms tick 만들기
 *
 *  왜 CTC 인가
 *    Normal(오버플로) 모드는 항상 256 을 세므로 원하는 주기를 맞추려면
 *    ISR 안에서 TCNT0 을 다시 채워야 하고, ISR 진입 지연만큼 오차가 쌓인다.
 *    CTC 는 OCR0 과 같아지는 순간 카운터가 스스로 0 이 되므로 오차가 없다.
 *
 *  주기 계산
 *    타이머 클럭 = 14,745,600 / 1024 = 14,400 Hz   (정확히 나누어떨어진다)
 *    f_tick      = 14,400 / (1 + 143) = 100 Hz     ->  10 ms,  오차 0 %
 *
 *  TCCR0 비트 (ATmega128 — 순서가 직관적이지 않다)
 *    bit7 FOC0 | bit6 WGM00 | bit5 COM01 | bit4 COM00
 *    bit3 WGM01| bit2 CS02  | bit1 CS01  | bit0 CS00
 *    CTC    : WGM01=1, WGM00=0
 *    /1024  : CS02:CS00 = 111
 *
 *  참고 : Timer0 의 분주 계열은 다른 타이머와 다르다 (/32 · /128 이 있다)
 *         1 · 8 · 32 · 64 · 128 · 256 · 1024
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "board.h"
#include "timer.h"

static volatile uint8_t  timer_event;
static volatile uint32_t timer_ms;

ISR(TIMER0_COMP_vect)               /* 10 ms 마다 */
{
    static uint8_t c50 = 0, c250 = 0;
    static uint8_t c1s = 0;

    timer_ms += TIMER_TICK_MS;
    timer_event |= TIMER_EVENT_10MS;

    if (++c50  >= 5)   { c50  = 0; timer_event |= TIMER_EVENT_50MS;  }
    if (++c250 >= 25)  { c250 = 0; timer_event |= TIMER_EVENT_250MS; }
    if (++c1s  >= 100) { c1s  = 0; timer_event |= TIMER_EVENT_1S;    }
}

void timer_init(void)
{
    timer_event = 0;
    timer_ms    = 0;

    TCNT0  = 0;
    OCR0   = 143;                                   /* 144 카운트 = 10 ms */
    TCCR0  = (1 << WGM01)                           /* CTC 모드 */
           | (1 << CS02) | (1 << CS01) | (1 << CS00);   /* 분주 1024 */
    TIMSK |= (1 << OCIE0);                          /* 비교일치 인터럽트 허용 */
}

uint8_t timer_get_events(void)
{
    uint8_t sreg = SREG;
    uint8_t ev;

    cli();
    ev = timer_event;
    timer_event = 0;
    SREG = sreg;

    return ev;
}

uint32_t timer_millis(void)
{
    uint32_t v;
    uint8_t sreg = SREG;

    cli();                  /* 32비트는 8비트 CPU 에서 네 번에 나눠 읽는다 */
    v = timer_ms;
    SREG = sreg;

    return v;
}
