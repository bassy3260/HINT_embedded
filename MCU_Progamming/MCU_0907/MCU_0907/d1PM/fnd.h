#ifndef FND_H_
#define FND_H_

#include <stdint.h>

void fnd_init(void);
void fnd_digit(uint8_t pos, uint8_t num);   /* pos자리에 num숫자 표시 */

#endif /* FND_H_ */