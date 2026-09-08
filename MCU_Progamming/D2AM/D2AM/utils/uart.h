#ifndef UART_H_
#define UART_H_
#include <stdint.h>

#define UART_BAUD 9600UL
#define UART_EVENT_RX (1 << 0)

void uart_init(void);
void uart_putchar(char c);          /* 한 글자 — 가장 기본 */
void uart_puts(const char *s);      /* 문자열 */
void uart_put_uint16(uint16_t v);   /* 숫자를 10진수로 */
void uart_put_hex8(uint8_t v);      /* 0x00 ~ 0xFF */
uint8_t uart_get_event(uint8_t *data); /* 수신 확인 */
#endif