# 실행 매뉴얼

> 공격·방해 실험 방법은 [EXPERIMENTS.md](EXPERIMENTS.md)

모든 명령은 **practice04 폴더에서** 실행한다.
```
cd D:\WSY\HINT_embedded\OTA_Security\20260928\practice04
```
필요한 것 : Python 3 (표준 라이브러리만), `openssl` 명령 (Git for Windows 에 포함, `openssl version` 으로 확인)

---

## 0. CA 와 서명 키 만들기 (맨 처음 한 번)
```
python prepare/make_ca.py      # Root CA
python prepare/make_fw_key.py  # 펌웨어 암호화 키
python prepare/make_keys.py 1  # 제작자 키 + CA 가 발급한 인증서 (키 버전 1)
```
| 파일 | 무엇 | 어디에 |
|---|---|---|
| `prepare/ca/ca.key` | CA **비밀키** (인증서 발급할 때만) | 개발자 PC (금고), git 제외 |
| `prepare/ca/ca.crt` → `device/trust/ca.crt` | CA 인증서 (기기가 믿는 유일한 것) | 출하 때 기기에 |
| `prepare/keys/publisher.key` | 제작자 **비밀키** (매번 서명) | 개발자 PC, git 제외 |
| `prepare/keys/publisher.crt` | 제작자 인증서 (CA 도장, **펌웨어 서명 전용**) | sign.py 가 서버에 올림 |
| `prepare/fw_key.hex` → `device/trust/fw_key.hex` | 펌웨어 암호화 키 (AES-256, **비밀**) | 개발자 PC + 출하 때 기기에. 서버 X, git 제외 |

- 이미 있으면 만들지 않고 멈춘다
- **제작자 키 교체** : `prepare/keys` 삭제 → `python prepare/make_keys.py 2` (버전 올림) → `python prepare/sign.py` (기기는 그대로)
- 인증서에 키 버전이 `OU=key-N` 으로 적힌다. 확인 : `openssl x509 -in prepare/keys/publisher.crt -noout -subject`
- **CA 를 새로 만들면** 기기의 `device/trust/ca.crt` 도 바뀐다 (= 기기 회수해서 교체한 것과 같음)

## 1. 펌웨어 만들기 + 서명
```
python prepare/make_firmware.py 1     # 버전 번호 (생략하면 1), 유효기간 기본 7일
python prepare/sign.py
```
- 새 버전을 배포할 때는 번호를 올린다 : `python prepare/make_firmware.py 2` → `sign.py`
- 유효기간(분)을 바꾸려면 두 번째 인자 : `python prepare/make_firmware.py 2 1` (1분), `-1` 이면 이미 만료
- **7일이 지나면 기기가 거부한다** → 새 펌웨어가 없어도 주기적으로 make_firmware + sign 다시
- `server/files/1` ~ `server/files/10` 이 생긴다 (`firmware.enc` **암호문** + `manifest.json`(하드웨어 이름 `board-N`, 버전, IV, 평문 해시, 서명 시각·만료 시각, 크기, 청크 크기, 청크별·전체 해시) + `manifest.json.sig`(서명)), 그리고 `server/files/publisher.crt`(제작자 인증서)
- manifest 를 고치거나 펌웨어를 다시 만들면 **sign.py 도 다시** 실행
- 평문 원본은 서버가 아니라 `prepare/plain/<타겟>.bin` 에 (결과 비교용)
- **다시 실행하면 펌웨어가 새로 랜덤 생성된다** → 기기에 받아둔 옛 청크는 해시가 안 맞아 자동으로 다시 받는다

## 2. 서버 켜기
터미널을 하나 따로 열어서
```
python server/server.py
```
- `서버 시작 : http://192.168.0.29:9999/` 가 뜨면 성공 (주소는 `server/server.py` 에서 변경)
- 켜진 상태로 두고, 기기는 다른 터미널에서 실행
- **방해 모드** (실험용) : `python server/server.py <모드>` → 펌웨어 요청의 30% 를 방해, 서버 창에 `[방해 : 모드]` 가 찍힘
  | 모드 | 방해 |
  |---|---|
  | `error` | 500 에러 |
  | `drop` | 절반만 보내고 끊기 |
  | `slow` | 15초 멈춤 (기기 타임아웃 10초) |
  | `corrupt` | 바이트 하나 뒤집기 |
  | `wrong` | 1바이트 밀린 구간 보내기 |
  | `full` | Range 무시하고 파일 전체 |
  | `drip` | 1초에 1바이트씩 흘려보내기 |
  | `mix` | 위 중 무작위 (drip 제외) |
  - 모드를 바꾸려면 서버를 끄고(Ctrl+C) 다시 켠다
- 확인 : 브라우저에서 http://192.168.0.29:9999/3/manifest.json → 해시 목록이 보이면 성공

## 3. 서버 끄기
- 서버 터미널에서 **Ctrl + C** (KeyboardInterrupt 메시지가 나오는 건 정상)
- 터미널을 이미 닫았거나 백그라운드로 켰다면 (PowerShell)
  ```
  Get-NetTCPConnection -LocalPort 9999 -State Listen | % { Stop-Process -Id $_.OwningProcess -Force }
  ```

## 4. 다운로드 (기기 실행)
```
python device/main.py 3        # 타겟 3 기기로 동작
```
- `기기 시각` → `제작자 인증서 OK (CA 확인)` → `키 버전 OK : key-1` → `manifest 서명 OK` → `하드웨어 확인 OK : board-3` → `만료 확인 OK` → `버전 확인 OK : v0 → v1` → `청크 0 받음 (검증 OK)` … `청크 15 받음 (검증 OK)` → `복호화 OK` → `부팅 칸 변경 : slot a (v1)` → `설치 완료`
- 결과 : `device/work/3/manifest.json(.sig)` (받은 manifest), `chunks/` (받은 청크), `slot_a.bin`·`slot_b.bin` (A/B 칸, **평문**, 설치할 때마다 번갈아), `slot_a.manifest.json(.sig)`·`slot_a.publisher.crt` (칸마다 보관한 서명된 manifest, 부팅 때 확인), `boot.json` (부팅할 칸 + 버전), `key_version.txt` (본 적 있는 가장 높은 키 버전), `bad_version.txt` (부팅에 실패했던 버전), `last_time.txt` (본 적 있는 가장 최근 서명 시각)
- 이미 같은 버전이 설치돼 있으면 `[거부] 새 버전이 아님` → 정상 (새 버전을 배포해야 받는다)
- **부팅 확인** : `python device/boot.py 3` → `부팅 : slot a, v1 (서명·해시 확인)` (보관한 manifest 를 CA 부터 다시 확인 → 칸의 펌웨어 해시 확인)
- **업데이트(두 번째 설치부터)** 는 `시험 부팅 대기` 로 끝난다 → `python device/boot.py 3` 으로 부팅해야 `확인 도장` 이 찍혀 확정. 확정 전에는 다음 업데이트를 안 받는다
- 원본과 같은지 확인 (PowerShell, 칸 이름은 boot.json 의 slot)
  ```
  (Get-FileHash device/work/3/slot_a.bin).Hash -eq (Get-FileHash prepare/plain/3.bin).Hash
  ```
  → `True`

### 실험용 스위치 (환경변수, 끝나면 `Remove-Item Env:이름` 으로 꼭 지우기)
| 이름 | 하는 일 | 예 |
|---|---|---|
| `CLOCK_SHIFT` | 기기 시계를 초 단위로 틀리게 | `$env:CLOCK_SHIFT=-3600` (1시간 과거) |
| `POWER_CUT` | 설치 중 이 바이트만 쓰고 전원 차단 흉내 | `$env:POWER_CUT=524288` (절반) |
| `CRASH_VERSION` | 부팅 때 이 버전 펌웨어는 켜지자마자 죽음 | `$env:CRASH_VERSION=2` |

### 이어받기 확인
1. 청크 하나 지우고, 설치 전 상태로 되돌리기 : `del device\work\3\chunks\5.bin, device\work\3\boot.json`
2. 다시 실행 : `python device/main.py 3` → `청크 5 받음 (검증 OK)` 만 나온다

## 5. 초기화
| 하고 싶은 것 | 지울 것 |
|---|---|
| 기기 3 처음부터 다시 받기 | `device/work/3` |
| 모든 기기 초기화 | `device/work` |
| (주의) `device/work/<타겟>` 을 지우면 설치 버전·키 버전도 0 = 공장 초기화 | |
| 펌웨어부터 새로 | `server/files`, `device/work` → 1번부터 다시 |
| 키부터 새로 | `prepare/ca`, `prepare/keys`, `device/trust` → 0번부터 다시 |

PowerShell : `Remove-Item -Recurse -Force device/work`

---

## 다른 PC(또는 보드)에서 기기 실행하기
서버 PC와 기기가 다를 때

1. 서버 PC IP 확인 : `ipconfig` → IPv4 주소 (예: 192.168.0.6)
2. 기기 쪽 `device/download.py` 의 주소 변경
   ```python
   SERVER = 'http://192.168.0.6:9999'
   ```
3. 기기 쪽에는 `device/` 폴더만 복사하면 된다 (prepare, server 불필요). `device/trust/ca.crt` 포함, 기기에도 `openssl` 필요
4. 기기 쪽에서도 `device/` 의 **상위 폴더**에서 실행 : `python device/main.py 3`

---

## 문제 해결
| 증상 | 원인 / 해결 |
|---|---|
| `ConnectionRefusedError` / `URLError` | 서버가 안 켜짐 → 2번. 다른 PC면 IP·방화벽(9999 포트 허용) 확인 |
| `HTTP Error 404` | 펌웨어·서명 없음 → 1번(make_firmware + sign) 실행 / 타겟 번호가 1~10 인지 확인 |
| `[부팅 실패] slot ... 펌웨어가 망가짐` | boot.json 이 가리키는 칸이 손상 → `device/work/<타겟>` 지우고 다시 설치 |
| `KeyError: 'slot'` / boot.json 없음 / `slot_a.manifest.json` 없음 | 옛 방식으로 설치된 기기 → `device/work/<타겟>` 지우고 다시 설치 |
| `[건너뜀] vN 은 부팅에 실패했던 버전` | 그 버전은 롤백된 적 있음 → 더 높은 버전을 배포 / 실험 중이면 `device/work/<타겟>/bad_version.txt` 삭제 |
| `[대기] 새 펌웨어가 아직 확인 전` | 업데이트 후 부팅을 안 함 → `python device/boot.py <타겟>` |
| `[거부] 새 버전이 아님` | 기기에 이미 그 버전 이상이 설치됨 → 번호를 올려 1번 다시, 또는 `device/work/<타겟>/boot.json` 삭제 |
| `KeyError: 'version'` 등 | 서버 파일이 옛 형식 → 1번(make_firmware + sign) 다시 |
| `[위조 의심] 제작자 인증서가 CA 로 확인되지 않음` | CA 를 새로 만들었는데 인증서는 옛 CA 것, 인증서 만료(1년), **펌웨어 서명 용도가 아닌 옛 인증서** → `prepare/keys` 지우고 make_keys → sign |
| `[거부] 만료된 manifest` | 서명한 지 7일 지남 → `make_firmware` + `sign` 다시 / 실험 중 `CLOCK_SHIFT` 가 남아 있으면 `Remove-Item Env:CLOCK_SHIFT` |
| `[거부] 폐기된 옛 키` | 기기가 더 높은 키 버전을 이미 봄 → 현재 키로 sign 다시 / 실험 중이면 `device/work/<타겟>/key_version.txt` 삭제 |
| `[거부] 제작자 인증서가 아님` | 인증서 이름이 `CN=OTA Publisher, OU=key-N` 이 아님 / 옛 인증서면 `prepare/keys` 지우고 `make_keys.py <버전>` → sign |
| `[거부] manifest 내용 이상` | 크기·청크 수가 안 맞거나 4 MiB 초과 → make_firmware + sign 다시 |
| `[실패] 복호화한 펌웨어 해시 불일치` | 기기의 `device/trust/fw_key.hex` 가 `prepare/fw_key.hex` 와 다름 |
| `[부팅 거부] ...` | 칸의 펌웨어·보관한 manifest 가 바뀜 → `device/work/<타겟>` 지우고 다시 설치 |
| `FileNotFoundError: prepare/fw_key.hex` | 0번 `make_fw_key.py` 안 함 |
| `[위조 의심] manifest 서명 검증 실패` | manifest 를 고친 뒤 sign.py 를 안 돌림, 또는 키를 새로 만들고 sign.py 를 안 돌림 → 1번 다시 |
| `FileNotFoundError` (openssl) | openssl 이 PATH 에 없음 → Git Bash 에서 실행하거나 PATH 에 `C:\Program Files\Git\mingw64\bin` 추가 |
| `OSError: ... 10048` (서버 켤 때) | 9999 포트를 이미 사용 중 → 3번으로 기존 서버 끄기 |
| `FileNotFoundError: server/files` | practice04 폴더가 아닌 곳에서 실행함 → `cd` 후 다시 |
| `ModuleNotFoundError: download` | `device/main.py` 를 경로 그대로 실행할 것 (`python device/main.py`) |
