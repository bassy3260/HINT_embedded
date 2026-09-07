# HINT_embedded

현대자동차그룹 **K-뉴딜 아카데미 HINT** 임베디드 AI SW 과정 실습 리포지토리입니다.
수업에서 다룬 C 언어, 포인터, MCU 펌웨어, 파이썬 예제 코드를 날짜별로 정리합니다.

## 디렉터리 구성

| 경로 | 내용 |
| --- | --- |
| `C/DAY1/` | C 언어 기초 — 변수와 자료형, `printf`/`scanf` 입출력, 전위·후위 증감 연산자, 문자열 배열, 성적표 출력 프로젝트 |
| `C/DAY2/` | 포인터 — 주소·역참조 개념, 포인터를 통한 값 변경, 함수 인자로 주소 전달(call by reference) |
| `MCU_Progamming/MCU_0907/` | ATmega128 펌웨어 — 레지스터 직접 접근 매크로(`REG8`)로 `DDRB`/`PORTB` 제어, Active-Low LED 8개 순차 점등 |
| `python/` | AI를 위한 파이썬 실습 (수업 자료는 `.gitignore` 처리) |

## 개발 환경

- **C 실습**: Visual Studio (MSVC) / C 표준 라이브러리. `scanf_s` 등 일부 예제는 Windows 전용
- **MCU 실습**: Microchip(Atmel) Studio 7, AVR-GCC 툴체인
  - 타깃 MCU: `ATmega128`
  - 프로그래머: AVRISP mkII (ISP, 125 kHz)
  - 프로젝트 파일: `MCU_Progamming/MCU_0907/MCU_0907.atsln`
- **파이썬 실습**: Python 3

## 빌드 & 실행

### C 예제

```powershell
# MSVC (개발자 명령 프롬프트)
cl C\DAY1\prj_ch2_add.cpp
.\prj_ch2_add.exe

# 또는 gcc
gcc C\DAY2\pointer_easy.cpp -o pointer_easy
.\pointer_easy
```

### MCU 예제

1. Microchip Studio에서 `MCU_Progamming/MCU_0907/MCU_0907.atsln` 열기
2. `Build > Build Solution` 으로 `.hex` 생성 (`MCU_0907/Debug/MCU_0907.hex`)
3. ISP 프로그래머로 ATmega128에 플래시

## 진행 이력

| 날짜 | 학습 내용 |
| --- | --- |
| DAY1 (08/24) | C 기초 문법, 입출력, 증감 연산자, 문자열 |
| DAY2 (08/25) | 포인터와 주소, call by reference |
| 09/07 | MCU 프로그래밍 — ATmega128 레지스터 제어와 LED 제어 |
