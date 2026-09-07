#ifndef BOARD_H_
#define BOARD_H_

#define F_CPU 14745600UL

/* ===== LED : D1~D8 = PB0~PB7 (Active-Low) ===== */
#define LED_DDR    DDRB
#define LED_PORT   PORTB
#define LED_COUNT  8

/* ===== 버튼 : SW2~SW5 = PE4~PE7 (누르면 0) ===== */
#define BTN_DDR       DDRE
#define BTN_PORT      PORTE
#define BTN_PIN       PINE
#define BTN_MASK      0xF0   /* PE4~PE7 = 상위 4비트 */
#define BTN_PIN_BASE  4      /* SW2가 붙은 핀 = PE4 */
#define BTN_NUM_BASE  2      /* app이 부르는 이름 시작 = SW2 */
#define BTN_COUNT     4

#define BTN_SW2  2
#define BTN_SW3  3
#define BTN_SW4  4
#define BTN_SW5  5

/* ===== FND : 세그먼트 PORTC, 자리선택 PD4~PD7 (Common Anode) ===== */
#define FND_SEG_DDR   DDRC
#define FND_SEG_PORT  PORTC     /* A~DP 세그먼트 */
#define FND_COM_DDR   DDRD
#define FND_COM_PORT  PORTD
#define FND_COM_MASK  0xF0      /* PD4~PD7 */
#define FND_COM_BASE  4         /* COM1 = PD4 */
#define FND_DIGITS    4
/* ===== 부저 : PG3 (능동형, 1=정지 0=울림) ===== */
#define BUZZER_DDR   DDRG
#define BUZZER_PORT  PORTG
#define BUZZER_BIT   PG3
#endif /* BOARD_H_ */