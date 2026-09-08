#ifndef BTN_H_
#define BTN_H_

#include <stdint.h>


void    btn_init(void);
uint8_t btn_is_pressed(uint8_t n);   /* 지금 눌려 있나 */
uint8_t btn_is_falling(uint8_t n);   /* 방금 눌린 순간인가 */

#endif /* BTN_H_ */