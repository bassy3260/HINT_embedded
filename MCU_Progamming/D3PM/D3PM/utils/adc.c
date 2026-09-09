/*
 * adc.c — ADC 드라이버 (두 채널 순환)
 *
 *  ADC 클럭
 *      逐次비교(SAR) 회로는 50 ~ 200 kHz 에서 10비트 정밀도를 보장한다.
 *      14.7456 MHz / 128 = 115.2 kHz   ->  ADPS2:ADPS0 = 111
 *      한 번 변환 = 13 ADC 클럭 = 13 / 115.2 kHz = 112.8 us
 *
 *  ADMUX
 *      REFS1:REFS0 = 01  : AVCC 기준 (보드에서 AREF 가 AVCC 에 묶여 있다)
 *      ADLAR       = 0   : 오른쪽 정렬 — ADC 매크로로 10비트를 한 번에 읽는다
 *      MUX4:MUX0         : 채널 번호 (VR=0, CDS=1)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "board.h"
#include "adc.h"

static volatile uint16_t adc_value[2];
static volatile uint8_t  adc_event;
static volatile uint8_t  adc_channel;
static volatile uint8_t  adc_busy;

ISR(ADC_vect)
{
    adc_value[adc_channel] = ADC;   /* ADCL 먼저 읽는 규칙을 매크로가 지켜 준다 */

    if (adc_channel == ADC_CH_VR)
        adc_event |= ADC_EVENT_VR;
    else
        adc_event |= ADC_EVENT_CDS;

    adc_busy = 0;
}

void adc_init(void)
{
    adc_value[ADC_CH_VR]  = 0;
    adc_value[ADC_CH_CDS] = 0;
    adc_event   = 0;
    adc_channel = ADC_CH_VR;
    adc_busy    = 0;

    DDRF  &= ~((1 << ADC_PIN_VR) | (1 << ADC_PIN_CDS));   /* 입력으로 */
    PORTF &= ~((1 << ADC_PIN_VR) | (1 << ADC_PIN_CDS));   /* 풀업은 끈다 */

    ADMUX  = (1 << REFS0);          /* AVCC 기준 · 우측 정렬 · 채널 0 */

    ADCSRA = (1 << ADEN)            /* ADC 켜기 */
           | (1 << ADIE)            /* 변환 완료 인터럽트 허용 */
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);   /* 분주 128 */
}

uint8_t adc_start(uint8_t channel)
{
    if ((channel != ADC_CH_VR) && (channel != ADC_CH_CDS))
        return 0;
    if (adc_busy)
        return 0;                   /* 이전 변환이 끝나지 않았다 */

    adc_busy    = 1;
    adc_channel = channel;

    /* REFS · ADLAR 은 보존하고 MUX4:0 만 바꾼다 — 1일차의 필드 쓰기 관용구 */
    ADMUX = (uint8_t)((ADMUX & 0xE0) | (channel & 0x1F));
    ADCSRA |= (1 << ADSC);          /* 단발 변환 시작 */

    return 1;
}

uint8_t adc_get_event(uint16_t value[2])
{
    uint8_t sreg = SREG;
    uint8_t ev;

    cli();
    ev = adc_event;
    if (ev & ADC_EVENT_VR)   value[ADC_CH_VR]  = adc_value[ADC_CH_VR];
    if (ev & ADC_EVENT_CDS)  value[ADC_CH_CDS] = adc_value[ADC_CH_CDS];
    adc_event = 0;
    SREG = sreg;

    return ev;
}

uint16_t adc_read_blocking(uint8_t channel)
{
    /* 인터럽트를 쓰지 않는 가장 단순한 형태 — 변환이 끝날 때까지 기다린다 */
    ADCSRA &= ~(1 << ADIE);
    ADMUX = (uint8_t)((ADMUX & 0xE0) | (channel & 0x1F));
    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))    /* ADSC 는 변환이 끝나면 0 이 된다 */
        ;

    return ADC;
}
