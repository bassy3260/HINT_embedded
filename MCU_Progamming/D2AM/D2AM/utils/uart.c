/*
 * uart.c — USART1 드라이버
 *
 *  보레이트 계산 (비동기 Normal 모드)
 *      UBRR = F_CPU / (16 x BAUD) - 1
 *           = 14,745,600 / (16 x 9600) - 1
 *           = 96 - 1  =  95            <- 정수로 정확히 떨어진다
 *      실제 보레이트 = 14,745,600 / (16 x 96) = 9600.0 bps   오차 0 %
 *
 *  14.7456 MHz 를 쓰는 이유가 바로 이것이다.
 *  (16 MHz 였다면 UBRR = 103, 오차 +0.16 %)
 *
 *  UCSR1C 는 ATmega128 에서 URSEL 이 없는 별도 주소다.
 *  (ATmega8/16 처럼 UBRRH 와 주소를 공유하지 않는다)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "board.h"
#include "uart.h"

#define UBRR_VALUE  ((F_CPU / (16UL * UART_BAUD)) - 1)   /* = 95 */

static volatile uint8_t rx_event;
static volatile uint8_t rx_data;

ISR(USART1_RX_vect)
{
    rx_data  = UDR1;            /* 읽어야 RXC1 플래그가 지워진다 */
    rx_event = UART_EVENT_RX;
}

void uart_init(void)
{
    rx_event = 0;

    UBRR1H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR1L = (uint8_t)(UBRR_VALUE);

    UCSR1B = (1 << TXEN1)       /* 송신 허용 */
           | (1 << RXEN1)       /* 수신 허용 */
           | (1 << RXCIE1);     /* 수신 완료 인터럽트 허용 */

    UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);   /* 8비트 · 패리티 없음 · 스톱 1 */
}

void uart_putchar(char c)
{
    while (!(UCSR1A & (1 << UDRE1)))     /* 송신 버퍼가 빌 때까지 기다린다 */
        ;
    UDR1 = (uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putchar('\r');          /* 터미널 줄바꿈 보정 */
        uart_putchar(*s++);
    }
}

void uart_put_uint16(uint16_t v)
{
    char buf[6];
    int8_t i = 0;

    if (v == 0) {
        uart_putchar('0');
        return;
    }
    while (v > 0) {
        buf[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (i > 0)
        uart_putchar(buf[--i]);          /* 거꾸로 담았으므로 뒤에서부터 */
}

void uart_put_hex8(uint8_t v)
{
    const char hex[] = "0123456789ABCDEF";

    uart_puts("0x");
    uart_putchar(hex[(v >> 4) & 0x0F]);
    uart_putchar(hex[v & 0x0F]);
}

uint8_t uart_get_event(uint8_t *data)
{
    uint8_t sreg = SREG;
    uint8_t ev;

    cli();
    ev = rx_event;
    if (ev & UART_EVENT_RX)
        *data = rx_data;
    rx_event = 0;
    SREG = sreg;

    return ev;
}
