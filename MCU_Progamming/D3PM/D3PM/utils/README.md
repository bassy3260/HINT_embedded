# 3일차 오후 실습 — 이 폴더만으로 빌드된다

ADC 와 상태 기계로 Auto Light ECU 를 완성한다
필요한 파일이 모두 이 폴더에 있고, `d3pm.atsln` 이 이 폴더의 솔루션이다.

## 실습마다 프로젝트에 넣을 파일

| 답안 | 주제 | 프로젝트에 추가할 파일 |
|---|---|---|
| `homework1.c` | 조도 센서 읽기 | `homework1.c` · `adc.c` · `led.c` · `timer.c` · `uart.c` · `adc.h` · `led.h` · `timer.h` · `uart.h` · `board.h` |
| `homework2.c` | Auto Light ECU | `homework2.c` · `adc.c` · `btn.c` · `led.c` · `pwm.c` · `timer.c` · `uart.c` · `adc.h` · `btn.h` · `led.h` · `pwm.h` · `timer.h` · `uart.h` · `board.h` |

`.h` 와 `board.h` 는 같은 폴더에 있으면 컴파일러가 찾으므로 프로젝트에 넣지
않아도 빌드된다. Solution Explorer 에 함께 넣어 두면 F12 로 따라가기 편하다.

## 중요 — 한 프로젝트에 `main` 은 하나만

`homework*.c` 마다 `main()` 이 있다. 전부 한 프로젝트에 넣으면
`multiple definition of 'main'` 으로 링크가 실패한다.
실습 하나를 고르고 나머지 `homework*.c` 는 프로젝트에서 빼 둔다
(파일은 디스크에 남으므로 Add ▸ Existing Item 으로 다시 넣으면 된다).

## 이 폴더에서 주의할 것

- `clcd.c` 는 이 폴더의 어느 실습도 쓰지 않는다 — 프로젝트에 넣지 않아도 된다.
- `clcd.h` 는 이 폴더의 어느 실습도 쓰지 않는다 — 프로젝트에 넣지 않아도 된다.

| 파일 | 계층 | 하는 일 |
|---|---|---|
| `homework*.c` | app | 무엇을 할지만 적는다 — 레지스터 이름이 나오지 않는다 |
| `*.h` | driver 인터페이스 | app 이 부를 함수 목록 |
| `*.c` (드라이버) | driver 구현 | 레지스터와 극성을 여기서 흡수한다 |
| `board.h` | 배선 | 어느 포트에 무엇이 달렸나 — 배선이 바뀌면 이 파일만 고친다 |

## 명령줄 빌드 확인

```
avr-gcc -mmcu=atmega128 -DF_CPU=14745600UL -Os -o homework1.elf homework1.c adc.c led.c timer.c uart.c
avr-gcc -mmcu=atmega128 -DF_CPU=14745600UL -Os -o homework2.elf homework2.c adc.c btn.c led.c pwm.c timer.c uart.c
```

Microchip Studio 에서는 **프로젝트 속성 ▸ Toolchain ▸ Symbols** 에
`F_CPU=14745600UL` 을 넣어 두는 것을 권장한다. `homework*.c` 의
`#define F_CPU` 는 그 파일에만 적용되므로 드라이버까지 지켜 주지 못한다.
