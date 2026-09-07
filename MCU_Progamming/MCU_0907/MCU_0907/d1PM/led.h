#ifndef LED_H_
#define LED_H_

#include <stdint.h>

void led_init(void);
void led_on(uint8_t n);
void led_off(uint8_t n);
void led_toggle(uint8_t n);

#endif /* LED_H_ */