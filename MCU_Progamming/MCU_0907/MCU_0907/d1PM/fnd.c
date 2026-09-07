#include <avr/io.h>
#include "board.h"
#include "fnd.h"

/* 공통 애노드(CA) — 세그먼트에 0을 주면 켜진다 */
/* bit0=A bit1=B bit2=C bit3=D bit4=E bit5=F bit6=G bit7=DP */
static const uint8_t FONT[10] = {
	0xC0, 0xF9, 0xA4, 0xB0, 0x99,   /* 0 1 2 3 4 */
	0x92, 0x82, 0xF8, 0x80, 0x90,   /* 5 6 7 8 9 */
};

void fnd_init(void)
{
	FND_SEG_DDR  = 0xFF;         /* 세그먼트 8핀 전부 출력 */
	FND_COM_DDR |= FND_COM_MASK; /* 자리선택 4핀 출력 */
}

void fnd_digit(uint8_t pos, uint8_t num)
{
	FND_COM_PORT &= ~FND_COM_MASK;               /* ① 모든 자리 끈다 */
	FND_SEG_PORT = FONT[num];                    /* ② 숫자 패턴 싣기 */
	FND_COM_PORT |= (1 << (FND_COM_BASE + pos));  /* ③ 그 자리만 켠다 */
}