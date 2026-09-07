#include <avr/io.h>
#include "board.h"
#include "buzzer.h"

void buzzer_init(void)
{
	BUZZER_PORT |= (1 << BUZZER_BIT);   /* 먼저 정지 상태로 (1=정지) */
	BUZZER_DDR  |= (1 << BUZZER_BIT);   /* PG3을 출력으로 */
}

void buzzer_on(void)  { BUZZER_PORT &= ~(1 << BUZZER_BIT); }   /* 0=울림 */
void buzzer_off(void) { BUZZER_PORT |=  (1 << BUZZER_BIT); }   /* 1=정지 */