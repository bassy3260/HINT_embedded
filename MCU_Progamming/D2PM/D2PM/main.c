/*
 * homework3.c
 *
 * 2일차 오후 실습 3 — 명령으로 LED 를 제어한다
 *
 *  실습 2 의 파서를 LED 제어 전용으로 다듬고, 명령마다 응답을 돌려준다.
 *  PC ↔ 보드 양방향이 완성되는 지점 — 3일차 통합 프로젝트의 디버깅 콘솔이 된다.
 *
 *  줄바꿈은 CR+LF 로 보낸다. uart_puts 가 LF 앞에 CR 을 붙여 주므로
 *  문자열에는 "\n" 만 적으면 된다.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "led.h"
#include "uart.h"

static void show_help(void)
{
    uart_puts("--- commands ---\n");
    uart_puts(" 1 : LED0 ON     0 : LED0 OFF\n");
    uart_puts(" a : ALL ON      s : ALL OFF\n");
    uart_puts(" n : 0~7 toggle   ? : help\n");
}

static void handle_cmd(uint8_t c)
{
    switch (c) {
    case '1':  led_on(0);        uart_puts("LED0 ON\n");   break;
    case '0':  led_off(0);       uart_puts("LED0 OFF\n");  break;

    /* led_write 는 "비트 1 = 점등" 이다 — 드라이버가 Active-Low 를 흡수한다 */
    case 'a':  led_write(0xFF);  uart_puts("ALL ON\n");    break;
    case 's':  led_write(0x00);  uart_puts("ALL OFF\n");   break;

    case '?':  show_help();                                break;

    default:
        if (c >= '0' && c <= '7') {           /* 개별 토글 */
            led_toggle((uint8_t)(c - '0'));
            uart_puts("TOGGLE ");
            uart_put_uint16((uint16_t)(c - '0'));
            uart_puts(" -> LED=");
            uart_put_hex8(led_read());
            uart_puts("\n");
        } else {
            uart_puts("? try ?\n");
        }
        break;
    }
}

int main(void)
{
    uint8_t rx;

    led_init();
    uart_init();
    sei();

    uart_puts("=== LED console ===\n");
    show_help();

    while (1) {
        if (uart_get_event(&rx) & UART_EVENT_RX)
            handle_cmd(rx);
    }
}

/*  확인할 것
 *    · 명령을 보낼 때마다 응답이 한 줄씩 돌아오는가
 *    · 'a' 로 여덟 개가 모두 켜지는가 (led_write(0xFF) = 전부 점등)
 *    · 알 수 없는 글자에 "? try ?" 가 나오는가
 *
 *  여기서 얻는 것
 *    · PC ↔ 보드 양방향 통신의 뼈대
 *    · 프로토콜 설계의 가장 작은 형태 (명령 · 응답)
 */
