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

#endif /* BOARD_H_ */