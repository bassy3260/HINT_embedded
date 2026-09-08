/*
 * led.h — LED 드라이버 (SDK ATMEGA128A : PB0~PB7, Active-Low)
 */

#ifndef LED_H_
#define LED_H_

#include <stdint.h>

void    led_init(void);              /* PORTB 를 출력으로, 전부 소등 */
void    led_on(uint8_t n);           /* n : 0 ~ 7 */
void    led_off(uint8_t n);
void    led_toggle(uint8_t n);
void    led_write(uint8_t pattern);  /* 비트 1 = 점등 (드라이버가 반전한다) */
uint8_t led_read(void);              /* 현재 점등 패턴 (비트 1 = 점등) */

#endif /* LED_H_ */
