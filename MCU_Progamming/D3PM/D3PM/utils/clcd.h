/*
 * clcd.h — HD44780 TEXT LCD 드라이버 (SDK ATMEGA128A)
 *
 *      RS   = PG0      0 = 명령,  1 = 데이터
 *      R/W  = PG1      0 = 쓰기,  1 = 읽기
 *      E    = PG2      상승 -> 하강 펄스로 전송
 *      D0~D7 = PA0 ~ PA7 (8비트 모드)
 *
 *  대비는 보드의 VR1 가변저항으로 맞춘다. 글자가 안 보이면 먼저 VR1 을 돌려 본다.
 */

#ifndef CLCD_H_
#define CLCD_H_

#include <stdint.h>

void lcd_init(void);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t d);
void lcd_clear(void);
void lcd_goto(uint8_t row, uint8_t col);          /* row 0~1, col 0~15 */
void lcd_puts(const char *s);
void lcd_put_uint16(uint16_t v, uint8_t width);   /* 오른쪽 정렬 · 앞을 공백 */

#endif /* CLCD_H_ */
