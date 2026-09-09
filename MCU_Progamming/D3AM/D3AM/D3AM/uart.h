/*
 * uart.h — USART1 드라이버 (SDK ATMEGA128A)
 *
 *  RXD1 = PD2, TXD1 = PD3  ->  FT232RL  ->  USB Micro 5P
 *  보드를 USB 케이블 하나로 PC 에 연결하면 가상 COM 포트가 생긴다.
 *
 *  14.7456 MHz 라 9600 baud 의 UBRR1 이 정확히 95 로 떨어진다 (오차 0 %).
 *  송신은 폴링, 수신은 인터럽트 — 로그 채널로 쓰기에 알맞은 조합이다.
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

#define UART_BAUD       9600UL
#define UART_EVENT_RX   (1 << 0)

void    uart_init(void);
void    uart_putchar(char c);              /* 폴링 송신 — 보낼 때까지 대기 */
void    uart_puts(const char *s);
void    uart_put_uint16(uint16_t v);       /* 0 ~ 65535 를 10진수로 */
void    uart_put_hex8(uint8_t v);          /* 0x00 ~ 0xFF */
uint8_t uart_get_event(uint8_t *data);     /* 수신 이벤트 확인 */

#endif /* UART_H_ */
