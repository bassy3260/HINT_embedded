/*
 * homework2.c
 *
 * 2일차 오후 실습 2 — 한 글자 명령 파서
 *
 *  PC 에서 글자 하나를 보내면 보드가 그에 맞게 동작하고 응답을 돌려준다.
 *  수신은 USART1_RX 인터럽트, 해석과 출력은 main — ISR 은 여전히 한 줄이다.
 *
 *  터미널 : 9600 · 8 · None · 1 · 흐름 제어 없음
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "led.h"
#include "btn.h"
#include "uart.h"

static void handle_cmd(uint8_t c)
{
    switch (c) {
    case '1': case '2': case '3':
        led_toggle((uint8_t)(c - '1'));
        break;

    case '0':
        led_write(0x00);                  /* 비트 1 = 점등 → 0x00 은 전부 소등 */
        break;

    case 'r':                             /* report */
        uart_puts("SW=");
        uart_put_uint16(btn_is_pressed(BTN_SW3));
        uart_puts(" LED=");
        uart_put_hex8(led_read());
        uart_puts("\n");
        break;

    case 'h':
        uart_puts("1/2/3 toggle  0 off  r report\n");
        break;

    default:
        uart_puts("? try h\n");
        break;
    }
}

int main2(void)
{
    uint8_t rx;

    led_init();
    btn_init();
    uart_init();
    sei();

    uart_puts("=== command parser ===\n");
    uart_puts("press h...\n");

    while (1) {
        if (uart_get_event(&rx) & UART_EVENT_RX)
            handle_cmd(rx);
    }
}

/*  확장해 볼 것
 *    · 대문자도 같이 받도록 ('1' 과 'A' 를 함께)
 *    · 잘못된 글자에 도움말을 자동으로 띄우기
 *    · 명령마다 응답을 돌려주면 "먹었는지" 를 눈으로 확인할 수 있다
 *
 *  주의
 *    · handle_cmd 를 ISR 안에서 부르면 안 된다 — uart_puts 는 블로킹이다
 *    · 한 글자만 담기므로 빠르게 붙여 보내면 놓친다 (4.7 절 링 버퍼)
 */
