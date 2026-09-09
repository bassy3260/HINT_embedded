/*
 * board.h — SDK ATMEGA128A 2020 (Rev 1.0) 보드 정의
 *
 *  회로도 : ../SDK_Atmega128.pdf
 *  퓨즈   : ../ATmega128 키트 퓨즈 설정.pptx
 *           LFUSE = 0x3F · HFUSE = 0xCF · EFUSE = 0xFF
 *           (CKSEL=1111 8~16MHz 외부 크리스털 · SUT=11 · CKOPT=0 풀스윙
 *            JTAGEN=1 → PF4~PF7 도 일반 GPIO · BOD 4.0V · SPIEN 유지)
 *
 *  이 파일 하나만 고치면 배선이 바뀌어도 나머지 코드는 그대로 쓴다.
 *
 *  ─────────────────────────────────────────────────────────────────────
 *  장치              연결                              극성 · 비고
 *  ─────────────────────────────────────────────────────────────────────
 *  LED  D1~D8        PB0 ~ PB7                         Active-Low
 *                    +5V - 1kΩ - LED - PBn             0 = 점등
 *  스위치 SW2~SW5    PE4 ~ PE7  (INT4 ~ INT7)          Active-Low
 *                    10kΩ 풀업 + 0.1uF                 누르면 0
 *  FND 세그먼트      PORTC   A=PC0 … G=PC6, DP=PC7     공통 애노드(CA)
 *                    각 세그먼트에 330Ω                0 = 점등
 *  FND 자리 COM1~4   PD4 ~ PD7                         1 = 그 자리 선택
 *  부저 BUZ          PG3   (반대쪽 1kΩ → +5V)          수동형 · 구형파를 줘야 운다
 *  TEXT LCD          RS=PG0 · R/W=PG1 · E=PG2          8비트 모드
 *                    D0~D7 = PA0 ~ PA7                 대비 VR1
 *  UART1 (USB)       RXD1=PD2 · TXD1=PD3               FT232RL → USB Micro
 *  UART0 (RS232)     RXD0=PE0 · TXD0=PE1               MAX232 → J6 (ISP 공유)
 *  PWM               OC3A = PE3  (Timer3)              D9 Active-Low
 *                                                       → Inverting 모드
 *  ADC  가변저항 VR3 PF0 = ADC0
 *  ADC  조도센서 CDS PF1 = ADC1                        밝을수록 값 ↑
 *  기준 전압         AREF ≈ AVCC = +5V                 REFS1:0 = 01
 *  확장 포트 J2      PF2 ~ PF7
 *  ISP  J4           MISO=PE1 · MOSI=PE0 · SCK=PB1 · /RST
 *  크리스털 X1       14.7456 MHz
 *  ─────────────────────────────────────────────────────────────────────
 */

#ifndef BOARD_H_
#define BOARD_H_

/* 반드시 <util/delay.h> 나 UART 계산보다 먼저 정의되어야 한다.
   프로젝트 속성(Symbols)에 F_CPU=14745600UL 을 넣는 것을 권장한다. */
#ifndef F_CPU
#define F_CPU 14745600UL
#endif

/* ---------------------------------------------------------- LED (PORTB) */
#define LED_DDR         DDRB
#define LED_PORT        PORTB
#define LED_COUNT       8
/* Active-Low : 0 을 쓰면 켜진다 */

/* ---------------------------------------------------------- 스위치 (PORTE) */
#define BTN_DDR         DDRE
#define BTN_PORT        PORTE
#define BTN_PIN         PINE
#define BTN_MASK        0xF0          /* PE4 ~ PE7 */
#define BTN_PIN_BASE    4             /* SW2 → PE4 이므로 핀 = 번호 + 2 */
#define BTN_NUM_BASE    2             /* app 은 SW2 ~ SW5 로 부른다 */
#define BTN_COUNT       4

/* ---------------------------------------------------------- FND */
#define FND_SEG_DDR     DDRC          /* 세그먼트 A~DP */
#define FND_SEG_PORT    PORTC
#define FND_COM_DDR     DDRD          /* 자리 선택 COM1~COM4 = PD4~PD7 */
#define FND_COM_PORT    PORTD
#define FND_COM_MASK    0xF0
#define FND_COM_BASE    4             /* COM1 = PD4 */
#define FND_DIGITS      4

/* ---------------------------------------------------------- 부저 */
#define BUZZER_DDR      DDRG
#define BUZZER_PORT     PORTG
#define BUZZER_BIT      PG3           /* Active-High · 수동형(구형파 필요) */

/* ---------------------------------------------------------- CLCD */
#define LCD_DATA_DDR    DDRA
#define LCD_DATA_PORT   PORTA
#define LCD_DATA_PIN    PINA
#define LCD_CTRL_DDR    DDRG
#define LCD_CTRL_PORT   PORTG
#define LCD_RS          PG0
#define LCD_RW          PG1
#define LCD_E           PG2

/* ---------------------------------------------------------- ADC */
#define ADC_PIN_VR      0             /* PF0 */
#define ADC_PIN_CDS     1             /* PF1 */

/* ---------------------------------------------------------- PWM */
#define PWM_DDR         DDRE
#define PWM_BIT         PE3           /* OC3A */

#endif /* BOARD_H_ */
