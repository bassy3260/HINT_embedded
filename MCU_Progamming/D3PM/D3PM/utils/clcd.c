/*
 * clcd.c — HD44780 TEXT LCD 드라이버
 *
 *  쓰기 한 번의 절차
 *      RS / R/W 를 정한다  ->  데이터를 PORTA 에 싣는다
 *      E = 1  ->  (최소 450 ns 유지)  ->  E = 0  ->  LCD 가 받아 간다
 *
 *  이 구현은 Busy Flag 를 읽지 않고 넉넉한 delay 로 기다린다.
 *  R/W 를 항상 0(쓰기)으로 두므로 PORTA 방향을 바꿀 필요가 없어 코드가 짧다.
 *  대신 명령마다 정해진 시간을 반드시 지켜야 한다.
 *      Clear Display / Return Home : 1.52 ms
 *      나머지 명령과 데이터        : 37 us
 */
#include <avr/io.h>
#include "board.h"        /* F_CPU 를 먼저 정의한다 — delay.h 보다 앞이어야 한다 */
#include <util/delay.h>
#include "clcd.h"

static void lcd_pulse(void)
{
    LCD_CTRL_PORT |=  (1 << LCD_E);
    _delay_us(1);                       /* E 최소 폭 450 ns — 넉넉하게 */
    LCD_CTRL_PORT &= ~(1 << LCD_E);
    _delay_us(1);
}

static void lcd_write(uint8_t rs, uint8_t value)
{
    if (rs)
        LCD_CTRL_PORT |=  (1 << LCD_RS);
    else
        LCD_CTRL_PORT &= ~(1 << LCD_RS);

    LCD_CTRL_PORT &= ~(1 << LCD_RW);    /* 항상 쓰기 */
    LCD_DATA_PORT  = value;
    lcd_pulse();
    _delay_us(50);                      /* 일반 명령 · 데이터 : 37 us 이상 */
}

void lcd_command(uint8_t cmd) { lcd_write(0, cmd); }
void lcd_data(uint8_t d)      { lcd_write(1, d); }

void lcd_init(void)
{
    LCD_DATA_DDR  = 0xFF;
    LCD_CTRL_DDR |= (1 << LCD_RS) | (1 << LCD_RW) | (1 << LCD_E);
    LCD_CTRL_PORT &= ~((1 << LCD_RS) | (1 << LCD_RW) | (1 << LCD_E));

    _delay_ms(40);                      /* 전원 안정화 — 15 ms 이상 */

    /* Wake-up : Function Set 을 세 번 보낸다 (데이터시트 초기화 시퀀스) */
    lcd_command(0x30);  _delay_ms(5);
    lcd_command(0x30);  _delay_us(150);
    lcd_command(0x30);  _delay_us(150);

    lcd_command(0x38);      /* Function Set : 8비트 · 2줄 · 5x8 폰트 */
    lcd_command(0x08);      /* Display OFF */
    lcd_command(0x01);      /* Clear Display */
    _delay_ms(2);           /* Clear 는 1.52 ms 걸린다 */
    lcd_command(0x06);      /* Entry Mode : 커서 오른쪽 이동 · 화면 고정 */
    lcd_command(0x0C);      /* Display ON · 커서 OFF · 깜빡임 OFF */
}

void lcd_clear(void)
{
    lcd_command(0x01);
    _delay_ms(2);
}

void lcd_goto(uint8_t row, uint8_t col)
{
    /* 1행 DDRAM 주소 0x00~, 2행 0x40~. 명령은 0x80 을 OR 한다 */
    uint8_t addr = (uint8_t)((row ? 0x40 : 0x00) + col);
    lcd_command((uint8_t)(0x80 | addr));
}

void lcd_puts(const char *s)
{
    while (*s)
        lcd_data((uint8_t)*s++);
}

void lcd_put_uint16(uint16_t v, uint8_t width)
{
    char buf[6];
    int8_t i = 0;

    if (v == 0)
        buf[i++] = '0';
    while (v > 0) {
        buf[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (width > i)                   /* 앞을 공백으로 채워 자릿수 고정 */
        { lcd_data(' '); width--; }
    while (i > 0)
        lcd_data((uint8_t)buf[--i]);
}
