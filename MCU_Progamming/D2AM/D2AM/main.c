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
#include "utils/led.h"
#include "utils/uart.h"

#define TXBUF 64

static volatile char txbuf[TXBUF];
static volatile uint8_t head,tail;

ISR(USART1_UDRE_vect)
{
	if(head == tail)
		UCSR1B &= ~(1<<UDRIE1);
	else
		UDR1 = txbuf[tail++ & (TXBUF -1)];
}

void uart_putchar_nb(char c){
	while((uint8_t)(head-tail)>=TXBUF);
	txbuf[head++ & (TXBUF -1)] = c;
	UCSR1B|=(1<<UDRIE1);
}
/* 문자열도 링 버퍼 버전을 쓰게 새로 만듦 */
void uart_puts_nb(const char *s)
{
	while (*s) {
		if (*s == 0x0A) uart_putchar_nb(0x0D);  /* LF 앞에 CR */
		uart_putchar_nb(*s++);
	}
}

static void handle_cmd(uint8_t c)
{
   switch (c) {
	   case '1': led_write(0xFF); uart_puts_nb("ALL ON\n");  break;
	   case '0': led_write(0x00); uart_puts_nb("ALL OFF\n"); break;
	   case '?': uart_puts_nb("1 = on  0 = off\n");          break;
	   default:  uart_puts_nb("? try ?\n");                  break;
   }
}

int main(void)
{
    uint8_t rx;
    led_init();
    uart_init();
    sei();
    uart_puts_nb("=== LED console ===\n");
    uart_puts_nb("1 = on  0 = off  ? = help\n");   /* 주석과 맞춤 */
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
