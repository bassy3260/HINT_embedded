#include <stdint.h>
// 주소 a를 받아서 그 주소를 만질 수 있는 형태로 바꿔주는 틀
// (volatile uint8_t *) a를 1바이트 짜리, 언제든 바뀔 수 있는 주소로 해석
// *(...) 그 주소의 내용을 실제로 읽거나 쓴다.
#define REG8(a) (*(volatile uint8_t *)(a))
#include <util/delay.h>
// 주소에 이름 붙이기
#define MY_DDRB REG8(0x37)
#define MY_PORTB REG8(0x38)

int volatile_test(void)
{
	MY_DDRB = 0xFF; /* PORTB 8핀 모두 출력으로 설정 */
	MY_PORTB = 0x00; /* Active-Low → LED 8개 점등 */
	while (1) {
		MY_PORTB = 0xFE;  _delay_ms(200);  // 1111 1110 → PB0
		MY_PORTB = 0xFD;  _delay_ms(200);  // 1111 1101 → PB1
		MY_PORTB = 0xFB;  _delay_ms(200);  // 1111 1011 → PB2
		MY_PORTB = 0xF7;  _delay_ms(200);  // 1111 0111 → PB3
		MY_PORTB = 0xEF;  _delay_ms(200);  // 1110 1111 → PB4
		MY_PORTB = 0xDF;  _delay_ms(200);  // 1101 1111 → PB5
		MY_PORTB = 0xBF;  _delay_ms(200);  // 1011 1111 → PB6
		MY_PORTB = 0x7F;  _delay_ms(200);  // 0111 1111 → PB7
	}
}