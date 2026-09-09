/*
 * fnd.c — 4자리 FND 다중화
 *
 *  글자 폰트 (공통 애노드 : 켜려는 세그먼트를 0 으로)
 *      bit0=A bit1=B bit2=C bit3=D bit4=E bit5=F bit6=G bit7=DP
 *      숫자 0 은 A~F 를 켜고 G·DP 를 끄므로  1100 0000 = 0xC0
 */
#include <avr/io.h>
#include "board.h"
#include "fnd.h"

static const uint8_t FONT[10] = {
    0xC0,   /* 0 */  0xF9,   /* 1 */  0xA4,   /* 2 */  0xB0,   /* 3 */
    0x99,   /* 4 */  0x92,   /* 5 */  0x82,   /* 6 */  0xF8,   /* 7 */
    0x80,   /* 8 */  0x90    /* 9 */
};

static uint8_t digit[FND_DIGITS];

void fnd_init(void)
{
    FND_SEG_PORT  = 0xFF;                     /* 모든 세그먼트 소등 */
    FND_SEG_DDR   = 0xFF;
    FND_COM_PORT &= ~FND_COM_MASK;            /* 모든 자리 선택 해제 */
    FND_COM_DDR  |= FND_COM_MASK;

    fnd_set_number(0);
}

void fnd_set_number(uint16_t value)
{
    if (value > 9999)
        value = 9999;

    digit[0] = (uint8_t)(value / 1000);
    digit[1] = (uint8_t)((value / 100) % 10);
    digit[2] = (uint8_t)((value / 10) % 10);
    digit[3] = (uint8_t)(value % 10);
}

void fnd_clear(void)
{
    FND_COM_PORT &= ~FND_COM_MASK;
    FND_SEG_PORT  = 0xFF;
}

void fnd_scan(void)
{
    static uint8_t pos = 0;

    /* ① 먼저 모든 자리를 끈다 — 세그먼트를 바꾸는 동안 잔상(고스트)이 남지 않게 */
    FND_COM_PORT &= ~FND_COM_MASK;

    /* ② 이번 자리의 숫자를 세그먼트에 싣는다 */
    FND_SEG_PORT = FONT[digit[pos]];

    /* ③ 그 자리만 켠다 — COM1 = PD4 */
    FND_COM_PORT |= (uint8_t)(1 << (FND_COM_BASE + pos));

    if (++pos >= FND_DIGITS)
        pos = 0;
}

void fnd_digit(uint8_t pos, uint8_t num)
{
    /* 타이머가 없는 1일차 실습용 — main 이 직접 한 자리씩 돌린다.
       순서는 fnd_scan 과 같다 : 끄고 → 세그먼트 → 그 자리만 켠다 */
    if (pos >= FND_DIGITS || num > 9)
        return;

    FND_COM_PORT &= ~FND_COM_MASK;
    FND_SEG_PORT  = FONT[num];
    FND_COM_PORT |= (uint8_t)(1 << (FND_COM_BASE + pos));
}
