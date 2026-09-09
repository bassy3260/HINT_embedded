/*
 * adc.h — ADC 드라이버 (SDK ATMEGA128A)
 *
 *      VR3 가변저항  ->  PF0 = ADC0
 *      CDS 조도센서  ->  PF1 = ADC1        (+5V - CDS - PF1 - 10kΩ - GND)
 *
 *  AREF 는 보드에서 AVCC(+5V) 에 묶여 있으므로 REFS1:0 = 01 (AVCC) 을 쓴다.
 *  변환 완료 인터럽트로 값을 받고, main 은 이벤트로 확인한다.
 */

#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

#define ADC_CH_VR       0             /* PF0 — 가변저항 */
#define ADC_CH_CDS      1             /* PF1 — 조도 센서 */

#define ADC_EVENT_VR    (1 << 0)
#define ADC_EVENT_CDS   (1 << 1)

void     adc_init(void);
uint8_t  adc_start(uint8_t channel);            /* 1 = 시작함, 0 = 변환 중 */
uint8_t  adc_get_event(uint16_t value[2]);      /* 채널별 최신값을 채운다 */
uint16_t adc_read_blocking(uint8_t channel);    /* 폴링 버전 — 실습 비교용 */

#endif /* ADC_H_ */
