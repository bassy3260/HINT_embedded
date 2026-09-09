/*
 * led.c — LED 드라이버 (SDK ATMEGA128A : PB0~PB7, Active-Low)
 *
 *  하드웨어 : +5V ─ 1kΩ ─ LED ─ PBn
 *             PBn = 0  ->  전류가 흘러 점등
 *             PBn = 1  ->  소등
 *
 *  Active-Low 반전을 드라이버가 흡수하므로 app 은 "켠다 / 끈다" 로만 생각한다.
 */
#include <avr/io.h>
#include "board.h"
#include "led.h"

void led_init(void)
{
    LED_PORT = 0xFF;          /* 먼저 소등 상태를 만들고 */
    LED_DDR  = 0xFF;          /* 그 다음 출력으로 — 순간적인 점등을 막는다 */
}

void led_on(uint8_t n)      { LED_PORT &= ~(1 << n); }   /* 0 = 점등 */
void led_off(uint8_t n)     { LED_PORT |=  (1 << n); }
void led_toggle(uint8_t n)  { LED_PORT ^=  (1 << n); }

void led_write(uint8_t pattern)
{
    LED_PORT = (uint8_t)~pattern;   /* 비트 1 = 점등으로 보이게 반전 */
}

uint8_t led_read(void)
{
    return (uint8_t)~LED_PORT;
}
