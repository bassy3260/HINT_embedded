/*
 * buzzer.c — 부저 드라이버 (수동형 · Active-High · PG3)
 *
 *  수동형이라 전압을 걸어 두는 것만으로는 울지 않는다. 구형파가 필요하다.
 *
 *  톤 만들기 — 비어 있는 Timer1 을 쓴다 (Timer0 = tick, Timer3 = PWM)
 *      분주 8        ->  14,745,600 / 8 = 1,843,200 Hz
 *      OCR1A = 899   ->  1,843,200 / (1 + 899) = 2,048 Hz 로 비교일치
 *      인터럽트마다 PG3 토글  ->  1,024 Hz 구형파,  오차 0 %
 *
 *  OC1A~OC1C 는 PB5~PB7 이라 부저(PG3)에 닿지 않는다.
 *  그래서 하드웨어 출력이 아니라 ISR 에서 핀을 직접 뒤집는다.
 *
 *  길이는 10 ms tick 이 센다 — app 이 buzzer_tick() 을 주기적으로 부른다.
 *  울리는 동안에도 CPU 는 자유롭다 (ISR 은 토글 한 줄이다).
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "board.h"
#include "buzzer.h"

#define BUZZER_TOP      899         /* 1,024 Hz 구형파 (분주 8) */

/* 남은 tick 수. buzzer_tick() 에서 줄이므로 volatile 이다. */
static volatile uint16_t beep_left;

ISR(TIMER1_COMPA_vect)              /* 2,048 Hz — 이 토글이 곧 소리다 */
{
    BUZZER_PORT ^= (1 << BUZZER_BIT);
}

static void tone_start(void)
{
    TCNT1  = 0;
    OCR1A  = BUZZER_TOP;
    TCCR1A = 0;                             /* 비 PWM — OC 핀은 쓰지 않는다 */
    TCCR1B = (1 << WGM12) | (1 << CS11);    /* CTC · 분주 8 */
    TIMSK |= (1 << OCIE1A);                 /* |= 필수 (Timer 0·1·2 공용) */
}

static void tone_stop(void)
{
    TIMSK  &= ~(1 << OCIE1A);
    TCCR1B  = 0;                            /* 클럭을 끊는다 */
    BUZZER_PORT &= ~(1 << BUZZER_BIT);      /* 정지 = LOW */
}

void buzzer_init(void)
{
    BUZZER_PORT &= ~(1 << BUZZER_BIT);      /* 먼저 정지 상태로 두고 */
    BUZZER_DDR  |=  (1 << BUZZER_BIT);      /* 그 다음 출력으로 바꾼다 */
    beep_left = 0;
    tone_stop();
}

void buzzer_on(void)  { tone_start(); }
void buzzer_off(void) { tone_stop(); }

void buzzer_beep(uint16_t ms)
{
    /* 10 ms tick 기준으로 올림 — 5 ms 를 넣어도 한 번은 울린다 */
    beep_left = (uint16_t)((ms + 9) / 10);
    if (beep_left)
        buzzer_on();
}

void buzzer_tick(void)              /* 10 ms 마다 부른다 */
{
    if (beep_left == 0)
        return;                     /* 울리는 중이 아니다 */
    if (--beep_left == 0)
        buzzer_off();               /* 마지막 tick 이면 끈다 */
}

void buzzer_beep_blocking(uint16_t ms)
{
    /* 타이머를 배우기 전 실습용 — delay 로 직접 파형을 만든다.
       1 kHz 니까 한 주기가 1 ms 다. 그동안 다른 일은 아무것도 못 한다. */
    uint16_t n = ms;

    while (n--) {
        BUZZER_PORT |=  (1 << BUZZER_BIT);
        _delay_us(500);
        BUZZER_PORT &= ~(1 << BUZZER_BIT);
        _delay_us(500);
    }
}
