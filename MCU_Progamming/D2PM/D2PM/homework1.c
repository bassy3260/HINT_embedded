/*
 * homework1.c
 *
 * 2일차 오후 실습 — 인터럽트를 로그로 검증
 *
 *  2일차 오후 : USART1 · 폴링 송신 · 인터럽트 수신
 *  보드       : RXD1=PD2, TXD1=PD3 -> FT232RL -> USB Micro 5P
 *  준비       : USB 케이블로 PC 연결 -> 장치 관리자에서 COM 번호 확인
 *               PuTTY  9600 / 8 / None / 1 / Flow control None
 *  인터럽트로 도는 코드는 눈으로 볼 수 없다. 그것을 보이게 만드는 것이 목적.
 */
#define F_CPU 14745600UL

#include <avr/interrupt.h>
#include "led.h"
#include "btn.h"
#include "uart.h"

int main1(void)
{
    uint8_t ev, rx;
    uint16_t count = 0;

    led_init();
    btn_init();
    btn_int_enable();
    uart_init();

    sei();

    uart_puts("\n=== SDK ATmega128A  UART1 log ===\n");
    uart_puts("1~3 : LED on   0 : all off   b : button state\n");

    while (1) {
        /* ① 버튼 이벤트를 로그로 남긴다 */
        ev = btn_get_events();
        if (ev) {
            uart_puts("BTN event = ");
            uart_put_hex8(ev);
            uart_puts("  count=");
            uart_put_uint16(++count);
            uart_puts("\n");

            if (ev & BTN_EVENT_SW2) led_toggle(0);
            if (ev & BTN_EVENT_SW3) led_toggle(1);
            if (ev & BTN_EVENT_SW4) led_toggle(2);
        }

        /* ② PC 에서 보낸 문자로 LED 를 제어한다 */
        if (uart_get_event(&rx) & UART_EVENT_RX) {
            uart_puts("RX '");
            uart_putchar((char)rx);
            uart_puts("'\n");

            switch (rx) {
            case '1': led_on(4); break;
            case '2': led_on(5); break;
            case '3': led_on(6); break;
            case '0': led_write(0x00); break;
            case 'b':
                uart_puts("  SW2=");  uart_put_uint16(btn_is_pressed(BTN_SW2));
                uart_puts("  SW3=");  uart_put_uint16(btn_is_pressed(BTN_SW3));
                uart_puts("  SW4=");  uart_put_uint16(btn_is_pressed(BTN_SW4));
                uart_puts("  SW5=");  uart_put_uint16(btn_is_pressed(BTN_SW5));
                uart_puts("\n");
                break;
            }
        }
    }
}

/*
 *  확인할 것
 *   1) 문자가 깨지면 F_CPU 와 보레이트 설정을 먼저 의심한다
 *      14.7456 MHz · 9600 -> UBRR1 = 95 (오차 0 %)
 *   2) uart_putchar 는 UDRE1 을 기다리는 폴링 송신이다.
 *      긴 문자열을 ISR 안에서 보내면 안 되는 이유를 설명해 보라
 *   3) 채터링을 로그로 세어 본다 — 한 번 눌렀을 때 count 가 몇 올라가는가
 */
