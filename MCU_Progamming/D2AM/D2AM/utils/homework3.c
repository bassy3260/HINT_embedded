/*
 * homework3.c
 *
 * 2일차 오후 실습 3 — 명령으로 LED 를 하나씩 제어한다
 *
 *  실습 2 는 여덟 개를 한꺼번에 켰다. 이번에는 번호로 하나씩 고른다.
 *  PC <-> 보드 양방향이 완성되는 지점 — 3일차 통합 프로젝트의 디버깅 콘솔이 된다.
 *
 *  줄바꿈은 CR+LF 로 보낸다. uart_puts 가 LF 앞에 CR 을 붙여 주므로
 *  문자열에는 "\n" 만 적으면 된다.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "led.h"
#include "uart.h"

static void handle_cmd(uint8_t c)
{
    if (c >= '0' && c <= '7') {          /* '0' ~ '7' -> LED 번호 */
        led_toggle((uint8_t)(c - '0'));
        uart_puts("TOGGLE ");
        uart_putchar((char)c);
        uart_puts("\n");
    } else {
        uart_puts("0~7 : LED toggle\n");
    }
}

int main5(void)
{
    uint8_t rx;

    led_init();
    uart_init();
    sei();

    uart_puts("=== LED console ===\n");
    uart_puts("0~7 : LED toggle\n");

    while (1) {
        if (uart_get_event(&rx) & UART_EVENT_RX)
            handle_cmd(rx);
    }
}

/*  확인할 것
 *    · '3' 을 두 번 보내면 D4 가 켜졌다 꺼지는가
 *    · 여러 번호를 보내면 각각 따로 기억되는가
 *    · 숫자가 아닌 글자에 도움말이 나오는가
 *
 *  여기서 얻는 것
 *    · PC <-> 보드 양방향 통신의 뼈대
 *    · 프로토콜 설계의 가장 작은 형태 (명령 · 응답)
 */
