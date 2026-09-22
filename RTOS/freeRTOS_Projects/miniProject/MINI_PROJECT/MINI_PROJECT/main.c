#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lcd.h"


/*================================================
 * 게임 설정
 *================================================*/

#define LCD_WIDTH       16
#define MAX_HP          8
#define GAME_TICK_MS    150
#define BUZZER_BIT  PG3   /* Active-Low · 능동형(켜기/끄기) */
/* 부저 BUZ PG3 (반대쪽 1kΩ → +5V)  능동형 · 켜면 고유음 */

/*================================================
 * 버튼 명령
 *================================================*/

typedef enum
{
    CMD_LEFT = 0,
    CMD_RIGHT,
    CMD_FIRE,
    CMD_RESTART

} GameCommand;


/*================================================
 * 게임 상태
 *================================================*/

typedef struct
{
    uint8_t player_x;

    uint8_t enemy_x;

    uint8_t bullet_x;

    uint8_t bullet_active;

    uint8_t hp;

    uint16_t score;

    uint8_t game_over;

} GameState;


/*================================================
 * FreeRTOS Queue
 *================================================*/

static QueueHandle_t xCommandQueue;


/*================================================
 * 게임 상태
 *================================================*/

static GameState game;


/*================================================
 * 함수 선언
 *================================================*/

static void vGameTask(void *pvParameters);
static void game_init(void);
static void game_update(void);
static void process_command(GameCommand cmd);
static void draw_game(void);
static void draw_hp(void);
static void game_over_screen(void);
static void button_interrupt_init(void);

/*====================
 * 소리 커스텀 
 *====================*/

typedef struct
{
	uint8_t  on;     // 1 = 소리, 0 = 쉼
	uint16_t ms;     // 길이, 0이면 끝
} Beep;

/* 예 : 맞았을 때 "삐-삑" */
static const Beep snd_damage[] = {
	{ 1, 150 }, { 0, 50 }, { 1, 50 },
	{ 0, 0 }
};
/* 게임오버 : "빠-바-바-밤———" (운명 교향곡 리듬) */
static const Beep snd_gameover[] = {
	{ 1, 120 }, { 0, 80 },
	{ 1, 120 }, { 0, 80 },
	{ 1, 120 }, { 0, 80 },
	{ 1, 800 },
	{ 0, 0 }
};

static void buzzer_init(void)
{
	DDRG  |= (1 << BUZZER_BIT);
	PORTG &= ~(1 << BUZZER_BIT);    // 0 = 꺼짐
}

static void buzzer_on(void)
{
	PORTG |= (1 << BUZZER_BIT);     // 1 = 소리
}

static void buzzer_off(void)
{
	PORTG &= ~(1 << BUZZER_BIT);    // 0 = 끔
}
/* 효과음 번호 */
typedef enum
{
	SND_DAMAGE = 0,     // 맞았을 때
	SND_GAMEOVER, 
	SND_COUNT
} SoundId;

/* 번호 → 악보 */
static const Beep * const sound_table[SND_COUNT] = {
	snd_damage,
	 snd_gameover  
};

static QueueHandle_t xSoundQueue;

/* 게임에서 부르는 함수 : 요청만 넣고 바로 돌아옴 */
static void play_sound(SoundId id)
{
	xQueueSend(xSoundQueue, &id, 0);
}

/* 연주 담당 Task */
static void vSoundTask(void *pvParameters)
{
	SoundId id;
	const Beep *snd;
	uint8_t i;

	(void)pvParameters;

	buzzer_init();

	while (1)
	{
		xQueueReceive(xSoundQueue, &id, portMAX_DELAY);   // 요청 올 때까지 대기

		snd = sound_table[id];

		for (i = 0; snd[i].ms != 0; i++)                  // 끝 표시(ms=0)까지
		{
			if (snd[i].on) buzzer_on();
			else           buzzer_off();

			vTaskDelay(pdMS_TO_TICKS(snd[i].ms));         // 이 동안 게임은 계속 돔
		}

		buzzer_off();
	}
}
/*================================================
 * 게임 초기화
 *================================================*/



static void game_init(void)
{
    game.player_x = 1;

    game.enemy_x = LCD_WIDTH - 1;

    game.bullet_x = 0;

    game.bullet_active = 0;

    game.hp = MAX_HP;

    game.score = 0;

    game.game_over = 0;


    draw_hp();


    lcd_clear();

    draw_game();
}


/*================================================
 * HP LED 표시
 *
 * Active Low
 *================================================*/

static void draw_hp(void)
{
	uint8_t lost;
	uint8_t pattern;

	lost = MAX_HP - game.hp;

	if (lost >= 8)
	{
		pattern = 0xFF;
	}
	else
	{
		pattern = (1U << lost) - 1U;     // lost = 0이면 0 → 전부 꺼짐
	}

	PORTB = ~pattern;
}


/*================================================
 * 명령 처리
 *================================================*/

static void process_command(GameCommand cmd)
{
    /*--------------------------------------------
     * RESTART는 Game Over 상태에서도 동작
     *-------------------------------------------*/

    if (cmd == CMD_RESTART)
    {
        game_init();

        return;
    }


    /*
     * Game Over이면
     * 나머지 명령은 무시
     */
    if (game.game_over)
    {
        return;
    }


    switch (cmd)
    {
        /*----------------------------------------
         * SW2 : 왼쪽 이동
         *---------------------------------------*/

        case CMD_LEFT:

            if (game.player_x > 0)
            {
                game.player_x--;
            }

            break;


        /*----------------------------------------
         * SW3 : 오른쪽 이동
         *---------------------------------------*/

        case CMD_RIGHT:

            if (game.player_x <
                (LCD_WIDTH - 1))
            {
                game.player_x++;
            }

            break;


        /*----------------------------------------
         * SW4 : 총알 발사
         *---------------------------------------*/

        case CMD_FIRE:

            /*
             * 기존 총알이 없을 때만 발사
             */
            if (!game.bullet_active)
            {
                /*
                 * 플레이어 오른쪽에서
                 * 총알 시작
                 */
                if (game.player_x <
                    (LCD_WIDTH - 1))
                {
                    game.bullet_x =
                        game.player_x + 1;

                    game.bullet_active = 1;
                }
            }

            break;


        default:

            break;
    }
}


/*================================================
 * 게임 상태 업데이트
 *================================================*/

static void game_update(void)
{
    /*
     * Game Over 상태
     */
    if (game.game_over)
    {
        return;
    }


    /*================================================
     * 총알 이동
     *================================================*/

    if (game.bullet_active)
    {
        /*
         * 현재 총알 위치에 적이 있는지
         * 먼저 확인
         */
        if (game.bullet_x ==
            game.enemy_x)
        {
            game.bullet_active = 0;

            game.score++;

            /*
             * 적 재생성
             */
            game.enemy_x =
                LCD_WIDTH - 1;
        }
        else
        {
            /*
             * 총알 오른쪽 이동
             */
            game.bullet_x++;


            /*
             * 이동 후 적 충돌
             */
            if (game.bullet_x ==
                game.enemy_x)
            {
                game.bullet_active = 0;

                game.score++;

                game.enemy_x =
                    LCD_WIDTH - 1;
            }
            else if (game.bullet_x >=
                     LCD_WIDTH)
            {
                game.bullet_active = 0;
            }
        }
    }


    /*================================================
     * 적 이동
     *
     * 오른쪽 → 왼쪽
     *================================================*/

    if (game.enemy_x > 0)
    {
        game.enemy_x--;
    }
    else
    {
        /*
         * 적이 왼쪽 끝까지 도달
         */
        if (game.hp > 0)
        {
            game.hp--;
        }
		play_sound(SND_DAMAGE); 
        draw_hp();


        game.enemy_x =
            LCD_WIDTH - 1;


        if (game.hp == 0)
        {
            game.game_over = 1;
			play_sound(SND_GAMEOVER); 
        }
    }


    /*================================================
     * 적과 플레이어 충돌
     *================================================*/

    if (game.enemy_x ==
        game.player_x)
    {
        if (game.hp > 0)
        {
            game.hp--;
        }
		
		play_sound(SND_DAMAGE); 
        draw_hp();


        game.enemy_x =
            LCD_WIDTH - 1;


        if (game.hp == 0)
        {
            game.game_over = 1;
			play_sound(SND_GAMEOVER); 
        }
    }
}


/*================================================
 * LCD 게임 화면
 *
 * 첫째 줄 : 게임
 *
 * P = Player
 * > = Bullet
 * E = Enemy
 *
 * 둘째 줄 : Score / HP
 *================================================*/

static void draw_game(void)
{
    uint8_t i;


    /*================================================
     * 첫 번째 줄
     *================================================*/

    lcd_gotoxy(0, 0);


    for (i = 0;
         i < LCD_WIDTH;
         i++)
    {
        /*
         * 플레이어 우선 표시
         */
        if (i ==
            game.player_x)
        {
            lcd_data('P');
        }

        /*
         * 적
         */
        else if (i ==
                 game.enemy_x)
        {
            lcd_data('E');
        }

        /*
         * 총알
         */
        else if ((game.bullet_active) &&
                 (i ==
                  game.bullet_x))
        {
            lcd_data('>');
        }

        else
        {
            lcd_data(' ');
        }
    }


    /*================================================
     * 두 번째 줄
     *================================================*/

    lcd_gotoxy(0, 1);


    lcd_string("Score:");


    /*
     * 3자리 점수
     */
    lcd_data(
        '0' +
        ((game.score / 100) % 10)
    );

    lcd_data(
        '0' +
        ((game.score / 10) % 10)
    );

    lcd_data(
        '0' +
        (game.score % 10)
    );


    lcd_string(" HP:");


    /*
     * HP 0~8
     */
    lcd_data(
        '0' + game.hp
    );


    /*
     * 남은 공간 삭제
     */
    lcd_string("   ");
}


/*================================================
 * GAME OVER 화면
 *================================================*/

static void game_over_screen(void)
{
    lcd_gotoxy(0, 0);

    lcd_string(
        "   GAME OVER    "
    );


    lcd_gotoxy(0, 1);

    lcd_string(
        "Score:"
    );


    lcd_data(
        '0' +
        ((game.score / 100) % 10)
    );

    lcd_data(
        '0' +
        ((game.score / 10) % 10)
    );

    lcd_data(
        '0' +
        (game.score % 10)
    );


    lcd_string(
        " SW6:RST"
    );
}


/*================================================
 * Game Task
 *================================================*/

static void vGameTask(void *pvParameters)
{
    GameCommand cmd;

    TickType_t xLastWakeTime;


    (void)pvParameters;


    /*--------------------------------------------
     * 초기화
     *-------------------------------------------*/

    game_init();


    xLastWakeTime =
        xTaskGetTickCount();


    while (1)
    {
        /*========================================
         * Queue에 들어온 버튼 명령 모두 처리
         *=======================================*/

        while (
            xQueueReceive(
                xCommandQueue,
                &cmd,
                0
            )
            == pdPASS
        )
        {
            process_command(cmd);
        }


        /*========================================
         * 게임 진행
         *=======================================*/

        if (!game.game_over)
        {
            game_update();

            draw_game();
        }
        else
        {
            game_over_screen();
        }


        /*========================================
         * 일정한 Game Tick
         *=======================================*/

        vTaskDelayUntil(
            &xLastWakeTime,
            pdMS_TO_TICKS(
                GAME_TICK_MS
            )
        );
    }
}


/*================================================
 * 버튼 외부 인터럽트 초기화
 *
 * SW2 = PE4 / INT4 : LEFT
 * SW3 = PE5 / INT5 : RIGHT
 * SW4 = PE6 / INT6 : FIRE
 * SW6 = PE7 / INT7 : RESTART
 *
 * 버튼은 Active Low라고 가정
 *
 * 평상시      HIGH
 * 버튼 누름   LOW
 *
 * Falling Edge
 *================================================*/

static void button_interrupt_init(void)
{
    /*================================================
     * PE4 ~ PE7 입력
     *================================================*/

    DDRE &=
        ~(
            (1 << PE4) |
            (1 << PE5) |
            (1 << PE6) |
            (1 << PE7)
         );


    /*================================================
     * 내부 Pull-up 활성화
     *================================================*/

    PORTE |=
        (1 << PE4) |
        (1 << PE5) |
        (1 << PE6) |
        (1 << PE7);


    /*================================================
     * INT4 Falling Edge
     *
     * ISC41 = 1
     * ISC40 = 0
     *================================================*/

    EICRB |=
        (1 << ISC41);

    EICRB &=
        ~(1 << ISC40);


    /*================================================
     * INT5 Falling Edge
     *
     * ISC51 = 1
     * ISC50 = 0
     *================================================*/

    EICRB |=
        (1 << ISC51);

    EICRB &=
        ~(1 << ISC50);


    /*================================================
     * INT6 Falling Edge
     *
     * ISC61 = 1
     * ISC60 = 0
     *================================================*/

    EICRB |=
        (1 << ISC61);

    EICRB &=
        ~(1 << ISC60);


    /*================================================
     * INT7 Falling Edge
     *
     * ISC71 = 1
     * ISC70 = 0
     *================================================*/

    EICRB |=
        (1 << ISC71);

    EICRB &=
        ~(1 << ISC70);


    /*================================================
     * 기존 Interrupt Flag 제거
     *================================================*/

    EIFR =
        (1 << INTF4) |
        (1 << INTF5) |
        (1 << INTF6) |
        (1 << INTF7);


    /*================================================
     * INT4 ~ INT7 Enable
     *================================================*/

    EIMSK |=
        (1 << INT4) |
        (1 << INT5) |
        (1 << INT6) |
        (1 << INT7);
}


/*================================================
 * SW2
 *
 * INT4 / PE4
 *
 * LEFT
 *================================================*/

ISR(INT4_vect)
{
    BaseType_t
        xHigherPriorityTaskWoken =
        pdFALSE;


    GameCommand cmd =
        CMD_LEFT;


    xQueueSendFromISR(
        xCommandQueue,
        &cmd,
        &xHigherPriorityTaskWoken
    );


    /*
     * 현재 사용하는 AVR FreeRTOS Port에서
     * 지원한다면 아래 사용
     *
     * portYIELD_FROM_ISR(
     *     xHigherPriorityTaskWoken
     * );
     */
}


/*================================================
 * SW3
 *
 * INT5 / PE5
 *
 * RIGHT
 *================================================*/

ISR(INT5_vect)
{
    BaseType_t
        xHigherPriorityTaskWoken =
        pdFALSE;


    GameCommand cmd =
        CMD_RIGHT;


    xQueueSendFromISR(
        xCommandQueue,
        &cmd,
        &xHigherPriorityTaskWoken
    );
}


/*================================================
 * SW4
 *
 * INT6 / PE6
 *
 * FIRE
 *================================================*/

ISR(INT6_vect)
{
    BaseType_t
        xHigherPriorityTaskWoken =
        pdFALSE;


    GameCommand cmd =
        CMD_FIRE;


    xQueueSendFromISR(
        xCommandQueue,
        &cmd,
        &xHigherPriorityTaskWoken
    );
}


/*================================================
 * SW6
 *
 * INT7 / PE7
 *
 * RESTART
 *================================================*/

ISR(INT7_vect)
{
    BaseType_t
        xHigherPriorityTaskWoken =
        pdFALSE;


    GameCommand cmd =
        CMD_RESTART;


    xQueueSendFromISR(
        xCommandQueue,
        &cmd,
        &xHigherPriorityTaskWoken
    );
}

ISR(TIMER3_COMPA_vect)
{
	PORTG ^= (1 << PG3);      // 인터럽트 올 때마다 PG3 뒤집기
}

/*================================================
 * main
 *================================================*/

int main(void)
{
    BaseType_t result;


    /*================================================
     * LED 초기화
     *
     * PORTB
     * Active Low
     *
     * 게임 HP 표시
     *================================================*/

    DDRB =
        0xFF;


    /*
     * 처음 모두 OFF
     */
    PORTB =
        0xFF;


    /*================================================
     * LCD 초기화
     *================================================*/

    lcd_init();

    lcd_clear();


    /*================================================
     * 버튼 명령 Queue 생성
     *
     * 최대 10개 명령 저장
     *================================================*/

    xCommandQueue =
        xQueueCreate(
            10,
            sizeof(GameCommand)
        );

	xSoundQueue = xQueueCreate(4, sizeof(SoundId));

	if (xSoundQueue == NULL)
	{
		PORTB = 0x00;
		while (1) {}
	}
    /*================================================
     * Queue 생성 실패
     *================================================*/

    if (xCommandQueue == NULL)
    {
        /*
         * Active Low
         *
         * LED 모두 ON =
         * 오류 표시
         */
        PORTB =
            0x00;


        while (1)
        {
        }
    }


    /*================================================
     * Game Task 생성
     *
     * Task 하나만 사용
     *================================================*/

    result =
        xTaskCreate(
            vGameTask,
            "Game",
            160,
            NULL,
            2,
            NULL
        );
	
	result = xTaskCreate(vSoundTask, "Sound", 100, NULL, 3, NULL);

	if (result != pdPASS)
	{
		PORTB = 0x00;
		while (1) {}
	}

    /*================================================
     * Task 생성 실패
     *================================================*/

    if (result != pdPASS)
    {
        PORTB =
            0x00;


        while (1)
        {
        }
    }


    /*================================================
     * INT4 ~ INT7 초기화
     *================================================*/

    button_interrupt_init();


    /*================================================
     * FreeRTOS Scheduler 시작
     *================================================*/

    vTaskStartScheduler();


    /*
     * Scheduler 시작 실패
     */
    PORTB =
        0x00;


    while (1)
    {
    }


    return 0;
}