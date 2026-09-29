# 실험 기록

단계마다 **무엇을 막으려 했고, 어떻게 실험했고, 무엇이 남았는지** 정리한다.
실행 방법(서버 켜기, 초기화 등)은 [MANUAL.md](MANUAL.md) 참고.

## 현재 방향
- 진행 중 : Step 16 이전 실습(09-23, 09-28) 대비 빠진 보안 항목 보완
- 남은 것 (하드웨어 영역, 원리만) : 신뢰 기준(ca.crt, fw_key, 버전·시각 기록)이 보통 파일, 중간 CA, 서명 확인한 파일을 디스크에서 다시 읽음(TOCTOU)
- 다음 후보 : 설치 후 재검증 (보안 부팅), 중간 CA (Root 는 금고에)
- 남은 확인 : Step 7 의 error·corrupt·wrong·full·mix (생략함), Step 8 drip 완료 후 설치본 확인
- 다음 후보 : 설치 중 전원 차단

## 요약
| 단계 | 추가한 것 | 막은 것 | 못 막은 것 |
|---|---|---|---|
| 1 | 청크 다운로드 + 이어받기 + 설치 | - (기준선) | 저장된 청크 변조 |
| 2 | manifest(청크별·전체 SHA-256) + 검증 | 저장본 변조, 서버 펌웨어 변조 | 서버에서 펌웨어 + manifest 같이 변조 |
| 2.1 | 구조 변경 : 서버는 원본만, 기기가 Range 로 청크 요청 | (동작 동일) | (동일) |
| 3 | manifest 서명 (Ed25519, 공개키는 기기에 내장) | 서버에서 펌웨어+manifest 변조, 공격자 키로 재서명 | **다른 타겟의 정상 펌웨어**를 넣으면 설치됨 |
| 4 | manifest 에 `hw` + 기기가 자기 하드웨어와 비교 | 다른 타겟의 정상 펌웨어, hw 값 변조 | 옛 버전의 정상 서명 펌웨어 (다운그레이드, 다음 단계) |
| 5 | manifest 에 `version` + 기기에 설치 버전 저장, 더 클 때만 설치 | 옛 버전 재사용, manifest 버전 변조 | **기기 저장소의 버전 값 변조** (보통 파일이라 되돌릴 수 있음) |
| 6 | 서버 방해 모드 (기기는 아직 그대로) | 나쁜 청크는 저장 안 됨 (drop) | 끊기면 **프로그램이 죽음** → 사람이 여러 번 다시 실행해야 완료 |
| 7 | 청크 단위 자동 재시도 (최대 5번, 1·2·4·8초 대기) | (직접 실험) | (직접 실험) |
| 8 | 요청 하나당 전체 시간 제한 30초 | 질질 끄는 서버 (drip) → 30초 뒤 재시도 | - |
| 9 | Root CA + 제작자 인증서, 기기는 CA 만 믿음 | 제작자 키 교체 시 기기 회수 불필요 (구조상) | **유출된 옛 키 + 옛 인증서** (CA 도장 진짜, 아직 유효) |
| 10 | 인증서에 키 버전(`OU=key-N`) + 기기가 본 최고 키 버전 기억 | 새 키를 본 기기에 유출된 옛 키로 공격 | **새 키를 한 번도 못 본 기기** (창고 재고, 공장 초기화) |
| 11 | manifest 만료(`expires`) + 기기 시각 = max(기기 시계, 본 적 있는 최근 서명 시각) | 인증서 만료 (확인), 나머지 (직접 실험) | (직접 실험) |
| 12 | 설치 중 전원 차단 흉내 (`POWER_CUT`), 기기는 아직 그대로 | - | 제자리 덮어쓰기라 **펌웨어가 반반** (벽돌), 버전 기록도 어긋남 |
| 13 | A/B 슬롯 + boot.json 한 번에 커밋 + 부팅 흉내(boot.py) | 설치 중 전원 차단 (구조상) | 다 썼고 해시도 맞는데 **실행하면 죽는** 펌웨어 |
| 14 | 시험 부팅 + 확인 도장 + 없으면 이전 칸으로 롤백 | 켜지자마자 죽는 펌웨어 → 이전 칸으로 복귀 | 롤백 후 **같은 v2 를 또 받아 무한 반복** |
| 15 | 부팅 실패 버전 기록(`bad_version.txt`) → 그 버전 이하는 청크 받기 전에 건너뜀 | (직접 실험) | (직접 실험) |
| 16 | 인증서 용도·이름, 보안 부팅, 암호화, Content-Range, manifest 구조 | (직접 실험) | (직접 실험) |

---

## Step 1 : 기준선 (보안 없음)

**구현**
- `prepare/make_firmware.py` : 타겟 1~10 랜덤 펌웨어(1 MiB) → 64 KiB 청크 16개로 잘라 `server/files/<타겟>/`
- `server/server.py` : 파일만 전달
- `device/` : 청크 받기 (이미 있는 청크는 건너뜀) → 이어붙여 설치

**실험**
| 실험 | 방법 | 결과 |
|---|---|---|
| 정상 | `python device/main.py 3` | 설치본 = 원본 |
| 이어받기 | `device/work/3/chunks/5.bin` 삭제 후 재실행 | 5번만 다시 받음 |
| 저장된 청크 변조 | `5.bin` 내용을 고치고 재실행 | **"설치 완료"** → 변조된 펌웨어 설치 |

**배운 점 / 남은 문제**
- 기기가 받은 것·저장된 것을 그대로 믿는다
- 막으려면 기기가 **정답(해시)** 을 알아야 한다

---

## Step 2 : 해시 검증

**구현**
- `prepare/make_firmware.py` : `manifest.json` 생성 = 청크별 SHA-256 목록 + 전체 SHA-256
- `device/verify.py` : 해시 비교
- `device/download.py` : 받은 청크는 **저장 전에** 검증 / 이어받기 때 저장된 청크도 **다시** 검증 → 손상 시 재다운로드
- `device/install.py` : 이어붙인 뒤 전체 해시 확인 후 설치

**실험**
| 실험 | 방법 | 결과 |
|---|---|---|
| 정상 | `python device/main.py 3` | `검증 OK` × 16 → 설치 완료 |
| 저장된 청크 변조 | `device/work/3/chunks/5.bin` 을 고치고 재실행 | `저장본 손상 → 다시 받음` → 설치 완료 |
| 서버 펌웨어 변조 | `server/files/3/firmware.bin` 의 7번 청크 구간을 고치고 (아래 명령), `device/work/3/chunks/7.bin` 삭제 후 실행 | `[실패] 청크 7 해시 불일치`, 설치 안 됨 |
| 서버 펌웨어 + manifest 변조 | 위 상태에서 `manifest.json` 의 7번 해시·전체 해시도 새 값으로 바꾸고 실행 | **설치 완료 (못 막음)** |

서버 펌웨어 7번 구간 변조 명령
```
python -c "f=open('server/files/3/firmware.bin','r+b'); f.seek(7*65536); f.write(b'EVIL')"
```

원상복구 : `server/files`, `device/work` 삭제 후 `python prepare/make_firmware.py`

**배운 점 / 남은 문제**
- 해시는 **우연한 손상·일부 변조**는 잡지만, 해시 자체를 **펌웨어와 같은 서버**에서 받으면 서버가 털렸을 때 둘 다 바뀐다
- "네트워크가 안전하다"는 가정은 서버 침해에는 도움이 안 됨
- 생각할 것 : manifest 가 서버가 아니라 **제작자가 만든 것**임을 기기가 믿으려면? 그 근거는 언제·어디서 기기에 들어가 있어야 할까?

---

## Step 2.1 : 구조 변경 (서버는 원본만)

**왜**
- 실제 OTA 서버/CDN 은 펌웨어 **파일 하나**만 두고, 기기가 **Range 요청**으로 필요한 구간만 받는다
- 청크 크기는 기기(RAM, 플래시 쓰기 단위)에 맞춰야 하므로 서버가 미리 자르지 않는다
- 다음 단계(서버 방해 동작)는 서버가 Range 를 직접 처리해야 넣기 쉽다

**구현**
- `prepare/make_firmware.py` : 청크 파일 생성 제거. `firmware.bin` + `manifest.json`(size, chunk_size, 청크별·전체 해시)만
- `server/server.py` : Range 요청(`bytes=시작-끝`)이면 그 구간만 206 으로 응답
- `device/download.py` : `Range: bytes={i*chunk_size}-{끝}` 로 청크 요청

**실험** : Step 2 실험 4개 모두 결과 동일 (정상 / 저장본 변조 복구 / 서버 변조 차단 / 서버+manifest 변조는 못 막음)

**남은 점**
- 청크 해시 단위(`chunk_size`)가 manifest 에 고정되어 있어 기기는 그 크기로 요청한다 (청크별 해시를 쓰는 대가)

---

## Step 3 : manifest 서명

**왜** : Step 2 에서 해시를 펌웨어와 같은 서버에서 받으니, 서버가 털리면 둘 다 바뀌었다.
→ 기기는 manifest 가 **서버가 아니라 제작자가 만든 것**임을 확인해야 한다.
→ 근거(제작자 공개키)는 서버에서 받으면 안 되고, **출하 때부터 기기에 들어 있어야** 한다.

**구현**
- `prepare/make_keys.py` : Ed25519 키 한 쌍. 비밀키 → `prepare/keys/`, 공개키 → `device/trust/`
- `prepare/sign.py` : `manifest.json` 을 비밀키로 서명 → `manifest.json.sig`
- `device/verify.py` : `verify_sig()` = 내장 공개키로 서명 확인 (openssl)
- `device/download.py` : manifest + sig 를 받아 **서명이 맞아야만** 내용을 읽는다 (그 다음은 Step 2 와 동일)

**실험**
| 실험 | 방법 | 결과 |
|---|---|---|
| 정상 | `python device/main.py 3` | `manifest 서명 OK` → 설치 완료 |
| 서버 펌웨어 + manifest 변조 (Step 2 구멍) | 펌웨어 7번 구간 변조 + manifest 해시도 새 값으로 | `[위조 의심] manifest 서명 검증 실패` |
| 공격자가 자기 키로 재서명 | `openssl genpkey -algorithm ed25519 -out evil.key` 후 그 키로 `manifest.json.sig` 다시 만들기 | `[위조 의심] manifest 서명 검증 실패` (기기는 제작자 공개키만 믿음) |
| **다른 타겟 펌웨어 넣기** | `server/files/7/*` 을 `server/files/3/` 에 복사 | **`manifest 서명 OK` → 설치 완료 (못 막음)** |
| 서명 파일 없음 | `server/files/3/manifest.json.sig` 삭제 | `HTTP Error 404` 로 멈춤 (설치 안 됨, 메시지는 불친절) |

재서명 명령 (공격자 흉내)
```
openssl genpkey -algorithm ed25519 -out evil.key
openssl pkeyutl -sign -inkey evil.key -rawin -in server/files/3/manifest.json -out server/files/3/manifest.json.sig
```
원상복구 : `python prepare/make_firmware.py` → `python prepare/sign.py`, `device/work` 삭제

**배운 점 / 남은 문제**
- 서명은 "제작자가 만든 것인가"만 증명한다. **"이 기기용인가"는 증명하지 않는다**
  - 타겟 7 펌웨어도 제작자가 정상 서명한 것이라 타겟 3 기기가 그대로 설치 → 실제라면 벽돌
- 비슷하게 **예전 버전**(제작자가 서명했던 옛 manifest)을 다시 줘도 통과할 것 (다운그레이드, 아직 실험 안 함)
- 생각할 것 : 기기가 "나한테 맞는 펌웨어"임을 확인하려면 manifest 에 무엇이 들어가야 할까? 그 값을 공격자가 못 바꾸게 하려면?

---

## Step 4 : 하드웨어 확인

**왜** : Step 3 에서 타겟 7 의 **정상 서명된** 펌웨어를 타겟 3 기기에 주니 그대로 설치됐다.
서명은 "누가 만들었나"만 증명하고 "누구용인가"는 증명하지 않는다.

**구현**
- `prepare/make_firmware.py` : manifest 에 `"hw": "board-N"` 추가 → manifest 전체가 서명되므로 이 값도 서명으로 보호됨
- `device/main.py` : `MY_HW = board-<타겟>` (이 기기의 하드웨어. 실제 기기라면 출하 때 저장된 값, 서버에서 받지 않음)
- `device/download.py` : 서명 확인 **다음에** `manifest['hw'] == MY_HW` 확인 → 다르면 청크를 받기 전에 거부

**실험** (practice04 폴더, PowerShell. 시작 전 `python prepare/make_firmware.py; python prepare/sign.py`)
| 실험 | 방법 | 결과 |
|---|---|---|
| 정상 | `Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3` | |
| 다른 타겟 펌웨어 넣기 (Step 3 구멍) | `Copy-Item server/files/7/* server/files/3/` 후 `Remove-Item -Recurse -Force device/work; python device/main.py 3` | `manifest 서명 OK` → `[거부] 다른 하드웨어용 펌웨어 : board-7 (이 기기 : board-3)`, 청크 안 받음 |
| manifest 의 hw 만 고치기 | 위 상태에서 `server/files/3/manifest.json` 의 `"board-7"` 을 메모장으로 `"board-3"` 으로 고치고 다시 실행 | (변형으로 실시) 타겟 3 의 진짜 manifest 에서 `"board-3"` → `"board-7"` 로 고침 → `[위조 의심] manifest 서명 검증 실패` (hw 비교 전에 서명에서 걸림) |
| hw 고치고 공격자 키로 재서명 | `openssl genpkey -algorithm ed25519 -out evil.key` → `openssl pkeyutl -sign -inkey evil.key -rawin -in server/files/3/manifest.json -out server/files/3/manifest.json.sig` → 다시 실행 | (미실시) |

원상복구 : `python prepare/make_firmware.py; python prepare/sign.py; Remove-Item -Recurse -Force device/work; Remove-Item evil.key`

**배운 점 / 남은 문제**
- 서명 = **"누가 만들었나"**, 하드웨어 확인 = **"누구용인가"**. 서로 다른 질문이라 둘 다 필요하다
  - 타겟 7 의 진짜 파일을 그대로 주면 서명은 OK 지만 hw 에서 거부 (Step 3 에서는 설치됐음)
- hw 를 manifest **안에** 넣으면 서명이 같이 보호한다 → hw 를 한 글자만 고쳐도 서명 검증에서 먼저 걸림
- 기기 쪽 기준값(`MY_HW`)은 서버에서 받지 않고 기기가 원래 갖고 있어야 한다 (공개키와 같은 원리)
- 남은 문제 : 제작자가 **예전에** 서명한, 같은 하드웨어용 **옛 버전**을 주면? 서명 OK, hw OK → 설치될 것 (다운그레이드)

---

## Step 5 : 버전 확인 (다운그레이드 방지)

**왜** : 서명 OK + hw OK 라도, 제작자가 **예전에** 서명한 옛 버전(취약점 있음)을 다시 주면 설치된다.
공격자는 위조할 필요 없이 예전에 공개된 진짜 파일을 보관했다가 쓰면 된다.

**구현**
- `prepare/make_firmware.py <버전>` : manifest 에 `"version": N` 추가 (서명으로 보호됨)
- `device/main.py` : 설치된 버전을 `device/work/<타겟>/version.txt` 에서 읽음 (없으면 0). **설치 성공 후에만** 새 버전으로 갱신
- `device/download.py` : 서명 → hw → `manifest['version'] > 설치된 버전` 확인. 아니면 청크 받기 전에 거부

**실험** (practice04 폴더, PowerShell)
```
# 준비 : v1 을 만들어 따로 보관 (공격자가 예전 파일을 저장해 둔 것)
python prepare/make_firmware.py 1; python prepare/sign.py
Copy-Item -Recurse server/files/3 v1_backup

# v2 배포 → 기기 업데이트
python prepare/make_firmware.py 2; python prepare/sign.py
Remove-Item -Recurse -Force device/work -EA 0
python device/main.py 3
```
| 실험 | 방법 | 결과 |
|---|---|---|
| 정상 (v2 설치) | 위 준비 명령 마지막 줄 | `버전 확인 OK : v0 → v2` → 설치 완료 |
| 옛 버전 다시 주기 | `Copy-Item v1_backup/* server/files/3/` → `python device/main.py 3` | 서명 OK, hw OK → `[거부] 새 버전이 아님 : v1 (설치된 버전 : v2)` |
| manifest 버전 숫자 고치기 | 위 상태에서 `server/files/3/manifest.json` 의 `"version": 1` 을 메모장으로 `3` 으로 → 다시 실행 | `[위조 의심] manifest 서명 검증 실패` (버전 비교 전에 서명에서 걸림) |
| 기기 저장소의 버전 고치기 | `v1_backup/*` 을 다시 복사해 원래 v1 로 되돌리고, `device/work/3/version.txt` 를 `0` 으로 고쳐서 → 다시 실행 | **v1 설치됨** (`version.txt` = 1, 설치본 = v1_backup 과 동일) → 못 막음 |

원상복구 : `python prepare/make_firmware.py 1; python prepare/sign.py; Remove-Item -Recurse -Force device/work, v1_backup`

**배운 점 / 남은 문제**
- 버전은 두 곳에 있다
  - manifest : **이 펌웨어의 버전** → 제작자만 바꿀 수 있음 (서명이 보호)
  - 기기 : **지금 설치된 버전** → 설치 성공할 때만 올림
  - 기기는 manifest 버전 > 설치된 버전 일 때만 설치
- 옛 버전의 **진짜** 파일(서명 OK, hw OK)도 버전 비교에서 거부됨
- manifest 의 버전 숫자를 고치면 서명에서 먼저 걸림 (hw 때와 같은 원리)
- **기기 쪽 버전 값을 되돌리면 뚫린다** → manifest 는 서명이 지켜주지만 기기의 기준값은 아무도 안 지켜줌
  - 실제 기기 : 보통 파일이 아니라 **되돌릴 수 없는 곳**에 저장 (eFuse / OTP, 보안 칩의 monotonic counter)
- 지금까지의 원칙 : 기기의 기준값(공개키, hw, 버전)은 **서버에서 받지 않는다**. 그중 바뀌는 값(버전)은 **되돌릴 수 없게** 저장한다

---

## Step 6 : 서버 방해 테스트 (견고성)

**왜** : 공격이 아니어도 실제 네트워크·서버는 끊기고, 느리고, 이상한 응답을 준다.
보안 검증(해시·서명)이 **나쁜 데이터를 막는 것**과, 그래도 **끝까지 받아내는 것**은 다른 문제다.

**구현 (서버만)**
- `server/server.py <모드>` : `firmware.bin` 요청의 30% 를 방해 (manifest 는 정상). 모드는 [MANUAL.md](MANUAL.md) 2번 참고
- 기기 코드는 **그대로** → 먼저 어디서 무너지는지 본다

**실험** (practice04 폴더, PowerShell)
```
# 준비 (한 번)
python prepare/make_firmware.py 1; python prepare/sign.py

# 서버 터미널 : 모드를 바꿔 가며
python server/server.py error

# 기기 터미널 : 매번 새 기기로 (설치 버전이 남아 있으면 "새 버전 아님" 으로 거부되므로)
Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3
```
| 모드 | 기기 결과 (어디서 멈추나, 무슨 메시지) | 다시 실행하면? | 설치본이 원본과 같나? |
|---|---|---|---|
| `error` | | | |
| `drop` | 청크 도중 `http.client.IncompleteRead` 로 **프로그램이 죽음** (Traceback). 끊긴 청크는 저장 안 됨 | 받아둔 다음 청크부터 이어받음. 끊기는 위치가 4~10 근처로 점점 뒤로 → **약 5번 실행**해서 완료 | 같음 (v1 설치) |
| `slow` | | | |
| `corrupt` | | | |
| `wrong` | | | |
| `full` | | | |
| `mix` | | | |

- "다시 실행하면?" 은 `device/work` 를 **지우지 않고** `python device/main.py 3` 를 한 번 더 (이어받기)
- 설치본 비교 : `(Get-FileHash device/work/3/firmware.bin).Hash -eq (Get-FileHash server/files/3/firmware.bin).Hash`

생각할 것
- 나쁜 데이터가 **설치된** 경우가 있나? (보안 문제) 아니면 **멈추기만** 하나? (견고성 문제)
- 사람이 다시 실행해 주지 않으면 기기는 어떻게 되나? 실제 기기라면?
- 기기가 스스로 해야 할 일은 무엇일까? 무한히 다시 시도해도 될까?

**배운 점 / 남은 문제**
- `drop` 만 실험하고 결론 : 보안(나쁜 데이터 설치)은 문제없지만, 끊기면 기기가 죽고 **사람이 다시 실행해야** 끝난다
- 실제 기기에는 다시 실행해 줄 사람이 없다 → 기기가 **스스로 다시 시도**해야 한다 (Step 7)
- 나머지 모드(error, slow, corrupt, wrong, full)는 Step 7 에서 재시도와 함께 확인

---

## Step 7 : 청크 자동 재시도

**왜** : Step 6 에서 서버가 끊으면 기기가 죽었고, 사람이 5번 다시 실행해서야 끝났다.

**정한 규칙**
| 질문 | 규칙 | 이유 |
|---|---|---|
| 무엇을 | 실패한 **그 청크만** | 받은 청크는 이미 저장됨 |
| 어떤 실패 | 끊김, 에러 응답, 타임아웃, **청크 해시 불일치** | "이번에 잘못 받음" → 다시 받으면 될 수 있음 |
| 재시도 안 하는 실패 | 서명 실패, hw 불일치, 옛 버전 | 다시 받아도 같음, 공격일 수 있음 → 바로 멈춤 |
| 몇 번 | 청크당 최대 5번, 넘으면 포기 | 무한 재시도 = 기기가 영원히 매달림, 배터리 |
| 간격 | 1, 2, 4, 8초 (점점 늘림) | 과부하 서버에 기기들이 한꺼번에 몰리지 않게 |

**구현**
- `device/download.py` : `get_chunk()` = 청크 하나를 받아 해시 확인, 실패하면 쉬었다 재시도. 5번 다 실패하면 `[실패]` 로 멈춤

**실험** (서버 터미널에서 모드를 바꿔 가며, 기기는 매번 `Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3`)
| 모드 | 한 번 실행에 끝나나? | 나온 재시도 메시지 | 설치본이 원본과 같나? |
|---|---|---|---|
| `drop` | | | |
| `error` | | | |
| `slow` | | | |
| `corrupt` | | | |
| `wrong` | | | |
| `full` | | | |
| `mix` | | | |

생각할 것
- `slow` 는 왜 오래 걸리나? 타임아웃 10초가 적당할까?
- 서버를 아예 꺼 놓고 실행하면? (manifest 받기는 재시도 대상이 아니다)
- 실행 도중 기기 쪽을 Ctrl+C 로 끊었다가 다시 실행하면?

**배운 점 / 남은 문제**
- (직접 기록)

---

## Step 8 : 질질 끌기 (drip)

**왜** : 기기의 `timeout=10` 은 "전체 시간" 이 아니라 **"다음 데이터가 올 때까지 기다리는 시간"** 이다.
- `slow` (15초 멈춤) : 10초 동안 아무것도 안 옴 → 포기 → 재시도 → 해결됨 (Step 7)
- `drip` (1초에 1바이트) : 계속 **조금씩은** 오니까 포기를 못 함 → 64 KiB 청크 하나에 약 18시간

**구현 (서버만)**
- `server/server.py drip` : 방해 걸린 청크를 1초에 1바이트씩 보냄 (`mix` 에는 포함 안 함)
- 기기 코드는 **그대로** → 정말 붙잡히는지 먼저 본다

**실험**
```
# 서버 터미널
python server/server.py drip

# 기기 터미널
Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3
```
| 확인할 것 | 결과 |
|---|---|
| 기기 화면은 어떻게 되나? (1~2분 지켜보기) | 0번 청크 받고 **1번 청크에서 멈춤** (40초 넘게 진행 없음) |
| 서버 화면에 `[방해 : drip]` 이 찍힌 청크 번호 | |
| 재시도 메시지가 나오나? | 안 나옴. 1초마다 1바이트씩 오니까 "10초 동안 아무것도 안 오면 포기" 규칙에 안 걸림 |

기기는 Ctrl+C 로 끊는다.

생각할 것 : "청크 하나는 **전체로** 30초 안에 받아야 한다" 는 규칙을 넣으면? 정상 청크는 몇 초 걸리나?

**구현 (막기)**
- `device/download.py` `get()` : 한 번에 다 읽지 않고 **온 만큼씩** 읽으면서 경과 시간 확인 → 30초 넘으면 `TimeoutError` → 기존 재시도로 넘어감
  - 10초 규칙 (`timeout=10`) : 멈춘 서버 / 30초 규칙 (`TOTAL_TIMEOUT`) : 질질 끄는 서버 → 둘 다 필요
- 재시도 메시지에 오류 내용도 표시 (`TimeoutError: 30초 안에 다 못 받음` 과 `TimeoutError: timed out` 구분)

**실험 (막은 뒤)** : 같은 명령으로 다시
| 확인할 것 | 결과 |
|---|---|
| drip 걸린 청크에서 몇 초 뒤 재시도하나? | 청크 2 에서 약 30초 뒤 포기 → 1초 쉬고 재시도 → 받음 |
| 나온 메시지 | `청크 2 받기 실패 (TimeoutError: 30초 안에 다 못 받음) → 재시도` → `청크 2 받음 (검증 OK)` |
| 한 번 실행에 끝나나? 설치본이 원본과 같나? | |

**배운 점 / 남은 문제**
- (직접 기록)

---

## Step 9 : CA (제작자 키 교체)

**왜** : 기기가 **제작자 공개키 하나**를 직접 들고 있으면, 제작자 키를 바꿔야 할 때(유출, 정기 교체, 제작자 여럿) 기기를 전부 회수해야 한다.
- 확인한 것 : 제작자가 새 키를 만들면 기기의 옛 공개키와 짝이 안 맞는다 (실험은 절차가 꼬여 기기 공개키까지 새것으로 바뀐 상태로 돌려서 통과함 → 개념으로 정리)

**구조** (도장 비유 : CA = 구청, 제작자 인증서 = 인감증명서)
```
Root CA 비밀키 (금고, 인증서 발급할 때만)
   └ 발급 → publisher.crt  "이 공개키는 진짜 제작자 것"   ← 서버로 배포해도 됨 (CA 도장이라 위조 불가)
               └ 제작자 비밀키로 서명 → manifest.json.sig
기기 : ca.crt 만 믿음 → publisher.crt 확인 → 그 안의 공개키로 manifest 서명 확인
```

**구현**
- `prepare/make_ca.py` : CA 키 + 자체 서명 인증서 (10년). `ca.crt` 를 `device/trust/` 에 (출하)
- `prepare/make_keys.py` : 제작자 키 + CA 가 발급한 `publisher.crt` (1년). 교체 = `prepare/keys` 지우고 다시 실행
- `prepare/sign.py` : 서명 + `publisher.crt` 를 `server/files/` 에 게시
- `device/verify.py` : `verify_cert()` = CA 로 인증서 확인, `verify_sig()` = 인증서 안의 공개키로 서명 확인
- `device/download.py` : 인증서 확인 → manifest 서명 → hw → version → 청크
- `device/trust/publisher_pub.pem` 은 더 이상 안 씀 (삭제)

**실험** (practice04 폴더, PowerShell. 서버는 정상 모드 `python server/server.py`)
| 실험 | 방법 | 결과 |
|---|---|---|
| 정상 | `Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3` | |
| **제작자 키 교체** (기기는 그대로) | `Remove-Item -Recurse -Force prepare/keys; python prepare/make_keys.py; python prepare/sign.py` → 기기 실행 | |
| 공격자가 자기 CA·인증서를 만들어 바꿔치기 | 아래 명령 → 기기 실행 | |
| 진짜 인증서는 두고 공격자 키로 서명만 | `server/files/publisher.crt` 를 진짜로 되돌린 뒤 (`python prepare/sign.py` 후) `evil.key` 로 3번 manifest 만 재서명 → 기기 실행 | |

공격자 흉내 (자기가 CA 인 척 인증서를 만들고 그 키로 서명)
```
openssl genpkey -algorithm ed25519 -out evil.key
openssl req -x509 -new -key evil.key -subj "/CN=OTA Publisher" -days 365 -out evil.crt
Copy-Item evil.crt server/files/publisher.crt
openssl pkeyutl -sign -inkey evil.key -rawin -in server/files/3/manifest.json -out server/files/3/manifest.json.sig
```
원상복구 : `python prepare/make_firmware.py 1; python prepare/sign.py; Remove-Item evil.key, evil.crt; Remove-Item -Recurse -Force device/work`

생각할 것 : 제작자 **옛 키가 유출**돼서 교체했다. 그런데 공격자가 **옛 publisher.crt + 옛 키로 서명**하면? (옛 인증서도 CA 도장은 진짜이고 아직 1년 안 지남)

**배운 점 / 남은 문제**
- 기기가 제작자 공개키를 직접 들면 키 교체 = 기기 회수. CA 를 두면 기기는 CA 만 믿고, 제작자 키는 CA 인증서로 바꿔 끼울 수 있다
- 남은 문제 : 유출된 **옛 키 + 옛 인증서** 로 서명하면 manifest 를 마음대로 만들 수 있다 (version 999, hw board-3) → 지금까지의 방어가 전부 뚫림
  - 해결책 후보 : ① 인증서 수명 짧게 ② 폐기 목록(CRL) ③ **키 버전** → ③ 선택 (Step 10)

---

## Step 10 : 키 버전 (유출된 옛 키 폐기)

**왜** : 옛 제작자 키가 유출되면 공격자는 옛 인증서(CA 도장은 진짜)로 원하는 manifest 를 서명할 수 있다.
manifest 의 `version` 은 공격자가 직접 쓰는 값이라 소용없다.

**원리** : Step 5 (펌웨어 버전) 와 같다. 대상이 펌웨어가 아니라 **키**
| | 적는 곳 | 누가 보호 | 기기가 기억 |
|---|---|---|---|
| 펌웨어 버전 (Step 5) | manifest | 제작자 서명 | 설치된 버전 |
| **키 버전** | 제작자 인증서 `OU=key-N` | **CA 도장** (옛 키로는 못 바꿈) | 본 적 있는 가장 높은 키 버전 |

**구현**
- `prepare/make_keys.py <키 버전>` : 인증서 이름에 `OU=key-N`
- `device/verify.py` `key_version()` : 인증서에서 키 버전 읽기
- `device/download.py` : CA 확인 다음에 `키 버전 < 기억한 값` 이면 거부
- `device/main.py` : `device/work/<타겟>/key_version.txt` 에 기억, 설치 성공 후 갱신

**실험** (practice04 폴더, PowerShell, 서버 정상 모드)
```
# 1) key-1 로 설치 + 공격자가 훔칠 key-1 을 따로 보관
Copy-Item -Recurse prepare/keys keys_v1
Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3

# 2) key-1 유출 → key-2 로 교체, 펌웨어 v2 배포 → 기기 업데이트
Remove-Item -Recurse -Force prepare/keys; python prepare/make_keys.py 2
python prepare/make_firmware.py 2; python prepare/sign.py
python device/main.py 3

# 3) 공격 : 훔친 key-1 로 manifest 를 만들어 서명 (버전도 마음대로 3)
python prepare/make_firmware.py 3
openssl pkeyutl -sign -inkey keys_v1/publisher.key -rawin -in server/files/3/manifest.json -out server/files/3/manifest.json.sig
Copy-Item keys_v1/publisher.crt server/files/publisher.crt
python device/main.py 3
```
| 실험 | 결과 |
|---|---|
| 1) key-1 설치 | `키 버전 OK : key-1` → 설치 완료 |
| 2) key-2 로 교체 후 업데이트 (기기 회수 없이 되나?) | `키 버전 OK : key-2`, v1 → v2 설치. **기기 회수 없이 됨** (CA 효과) |
| 3) 훔친 key-1 로 공격 | 제작자 인증서 OK (CA 도장은 진짜) → `[거부] 폐기된 옛 키 : key-1 (기기가 본 최신 키 : key-2)` |
| 4) 3) 상태에서 `device/work` 를 지우고 (key-2 를 **한 번도 못 본** 새 기기) 다시 실행 | 키 버전 OK → 공격자 v3 **설치됨** (다시 실행하면 `새 버전이 아님 : v3 (설치된 버전 : v3)`) |

원상복구 : `python prepare/make_firmware.py 1; python prepare/sign.py; Remove-Item -Recurse -Force keys_v1, device/work`
(키는 key-2 로 남음. key-1 로 돌리려면 `prepare/keys` 지우고 `make_keys.py 1` → sign)

**배운 점 / 남은 문제**
- 키 버전은 **CA 도장** 안에 있어서 옛 키를 가진 공격자도 못 바꾼다 → 새 키를 본 기기는 옛 키를 거부
- CA 덕분에 키를 교체해도 기기 회수 없이 업데이트됨 (Step 9 의 효과를 여기서 확인)
- 한계 : 기기가 새 키를 **한 번은 봐야** 옛 키를 거부 → 오래 꺼져 있던 기기·창고 재고·공장 초기화 기기는 뚫림
  - 보완 : 출하 전에 최신 키 버전 기록, 키 버전을 초기화로도 안 지워지는 곳(eFuse)에, 인증서 수명 짧게 → 여러 방법을 같이 쓴다
- 실험 중 실수에서 배운 것 : 서로 다른 키 두 개에 **같은 버전 번호**(key-3 두 번)를 붙이면 기기가 구분 못 해 옛 키가 통과 → 키 버전은 교체할 때마다 반드시 올린다

---

## Step 11 : manifest 만료 + 기기 시각

**왜**
- 지금까지 점검 (질문 3개) : 롤백 방지 O (펌웨어·키 버전, 단 기기 파일은 고칠 수 있음) / 인증서 만료 O (`openssl verify` 가 자동 확인) / 전체 해시 O (설치 전)
- 빠진 것 1 : **manifest 에 만료가 없다** → 공격자가 예전 manifest(진짜 서명, 취약한 v2)를 계속 주면 최신(v3)이 있어도 v2 설치 / 새 버전을 숨겨도 모름
- 빠진 것 2 : 만료를 확인하려면 **기기가 "지금" 을 알아야** 한다 → 기기 시계는 없거나 틀리거나 되돌려질 수 있음 (인증서 만료도 같은 문제)

**정한 방법**
- manifest 에 `issued_at`(서명 시각), `expires`(기본 7일 뒤) → 서명으로 보호. 제작자는 새 펌웨어가 없어도 **주기적으로 재서명**
- 기기 시각 = **max(기기 시계, 본 적 있는 가장 최근 `issued_at`)** → 시계를 과거로 돌려도 이미 본 시각보다 뒤로는 안 감 (펌웨어·키 버전과 같은 원리)
- 인증서 유효기간 확인도 같은 기기 시각으로 (`openssl verify -attime`)
- 한계 : 시계를 **미래**로 돌리는 건 못 막음 (대신 만료가 빨리 걸릴 뿐이라 공격자 이득 없음)

**구현**
- `prepare/make_firmware.py <버전> [유효기간(분)]` : `issued_at`, `expires` 추가
- `device/main.py` : `last_time.txt` 기억 (설치 성공 때만 갱신, 간단히 하기 위해), `CLOCK_SHIFT` 환경변수로 기기 시계를 틀리게 (실험용)
- `device/download.py` : 인증서 확인에 기기 시각 사용 → hw 다음에 `now > expires` 면 거부
- `device/verify.py` : `verify_cert(cert, now)`

**실험** (practice04 폴더, PowerShell, 서버 정상 모드)
```
# 준비 1 : 공격자가 v2 manifest (1분짜리) 를 저장해 둠
python prepare/make_firmware.py 2 1; python prepare/sign.py
Copy-Item -Recurse server/files/3 saved_v2

# 준비 2 : 1분 넘게 기다린 뒤, 기기가 최근 서명된 v1 을 설치 (기기가 "최근 시각" 을 기억)
#          (순서가 어색하지만 시간 규칙만 보기 위한 실험)
python prepare/make_firmware.py 1; python prepare/sign.py
Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3
```
| 실험 | 방법 | 결과 |
|---|---|---|
| 정상 | 준비 2 마지막 줄 | |
| 만료된 manifest | `Copy-Item saved_v2/* server/files/3/; python device/main.py 3` | |
| 기기 시계를 1시간 과거로 | `$env:CLOCK_SHIFT=-3600; python device/main.py 3; Remove-Item Env:CLOCK_SHIFT` | |
| 시계 과거 + **기억 없는 새 기기** | `$env:CLOCK_SHIFT=-3600; Remove-Item -Recurse -Force device/work; python device/main.py 3; Remove-Item Env:CLOCK_SHIFT` | |
| 인증서 만료 (시계 2년 뒤) | `python prepare/make_firmware.py 1 2000000; python prepare/sign.py` 후 `$env:CLOCK_SHIFT=63072000; Remove-Item -Recurse -Force device/work; python device/main.py 3; Remove-Item Env:CLOCK_SHIFT` | 기기 시각 2028-09-27 → `[위조 의심] 제작자 인증서가 CA 로 확인되지 않음 (또는 만료)` (인증서는 2027-09-28 만료). manifest 확인까지 가지 않고 멈춤 |

- 시계 과거 실험은 준비 1 에서 **1시간 안에** 할 것 (그 뒤면 1시간 되돌려도 이미 만료)
- 마지막 실험의 `2000000` 분 ≈ 3.8년 : manifest 는 안 만료되게 해서 **인증서** 만료만 보기

원상복구 : `Remove-Item Env:CLOCK_SHIFT -EA 0; python prepare/make_firmware.py 1; python prepare/sign.py; Remove-Item -Recurse -Force saved_v2, device/work`

**배운 점 / 남은 문제**
- (직접 기록)

---

## Step 12 : 설치 중 전원 차단

**왜** : 지금까지의 방어(해시, 서명, 재시도…)는 전부 **설치 전** 확인이다.
설치(쓰기) 도중 전원이 나가면? 다운로드 중 끊김과 달리 **지금 돌아가는 펌웨어**가 망가진다.

| | 다운로드 중 끊김 | 설치 중 전원 차단 |
|---|---|---|
| 망가지는 것 | 받던 청크 하나 (따로 저장) | **기존 펌웨어** (제자리 덮어쓰기) |
| 원인 | 서버·네트워크 | 기기 안 |

**구현 (흉내만)**
- `device/install.py` : 실제 플래시처럼 기존 `firmware.bin` 위에 **제자리로** 덮어쓰기
- `POWER_CUT=<바이트>` 환경변수 : 그만큼만 쓰고 뒷정리 없이 종료 (`version.txt` 등도 못 씀)
- 기기 코드의 설치 방식은 **그대로** → 먼저 정말 망가지는지 본다

**실험** (practice04 폴더, PowerShell, 서버 정상 모드)
```
# 준비 : v1 설치 + 설치된 v1 을 비교용으로 복사
python prepare/make_firmware.py 1; python prepare/sign.py
Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3
Copy-Item device/work/3/firmware.bin installed_v1.bin

# v2 배포 → 설치 도중 절반(524288 바이트)에서 전원 차단
python prepare/make_firmware.py 2; python prepare/sign.py
$env:POWER_CUT=524288; python device/main.py 3; Remove-Item Env:POWER_CUT
```
확인 명령
```
$fw = (Get-FileHash device/work/3/firmware.bin).Hash
$fw -eq (Get-FileHash installed_v1.bin).Hash              # v1 인가?
$fw -eq (Get-FileHash server/files/3/firmware.bin).Hash   # v2 인가?
Get-Content device/work/3/version.txt                     # 기기가 아는 버전
```
| 확인할 것 | 결과 |
|---|---|
| 기기 출력 마지막 줄 | `[전원 차단] 524288 바이트 쓰고 꺼짐` |
| `firmware.bin` 은 v1? v2? | **둘 다 아님** : 앞 절반 = v2, 뒤 절반 = v1 |
| `version.txt` 는? | **1** (받아 둔 `manifest.json` 은 v2) → 기기는 v1 이라고 믿지만 실제 펌웨어는 반반 |
| `python device/main.py 3` 다시 실행하면? | (미확인) |

생각할 것 : 여기서는 다시 실행하면 되지만, **실제 기기라면** 꺼진 뒤 다시 켜질 때 무엇으로 부팅할까? 다시 실행할 기회가 있을까?

원상복구 : `Remove-Item Env:POWER_CUT -EA 0; python prepare/make_firmware.py 1; python prepare/sign.py; Remove-Item -Recurse -Force installed_v1.bin, device/work`

**배운 점 / 남은 문제**
- 원인 1 : **유일한 펌웨어를 제자리에서 덮어씀** → 쓰는 도중엔 멀쩡한 펌웨어가 하나도 없다
- 원인 2 : **펌웨어 쓰기와 버전 기록이 따로** → 사이에 꺼지면 서로 어긋난다
- PC 에선 다시 실행하면 되지만 실제 기기는 반반짜리로 부팅하다 실패 → 다시 실행할 기회가 없다
- 해결 : A/B 슬롯 (Step 13)

---

## Step 13 : A/B 슬롯

**왜** : Step 12 에서 제자리 덮어쓰기 도중 꺼지니 펌웨어가 반반이 됐고, 버전 기록도 어긋났다.

**구조**
```
slot_a.bin : v1 (지금 부팅하는 칸)     ← 설치 중에 절대 안 건드림
slot_b.bin : v2 를 여기에 씀           ← 꺼져도 A 는 멀쩡
다 쓰고 다시 읽어 해시 확인 → boot.json 을 한 번에 교체 {"slot": "b", "version": 2, "sha256": ...}
```
- 원인 1 해결 : 쓰는 칸 ≠ 부팅 칸
- 원인 2 해결 : 부팅 칸 + 버전을 **boot.json 하나에** 두고, 임시 파일에 쓴 뒤 **이름 바꾸기**(`os.replace`)로 한 번에 교체 → "반쯤 바뀐" 상태가 없다

**구현**
- `device/install.py` : 부팅 칸 반대쪽에 쓰기 → 다시 읽어 확인 → boot.json 커밋
- `device/main.py` : 설치 버전을 `version.txt` 대신 `boot.json` 에서 읽음
- `device/boot.py` : 부팅 흉내. boot.json 이 가리키는 칸의 해시를 확인하고 "부팅"

**실험** (practice04 폴더, PowerShell, 서버 정상 모드)
```
# 준비 : v1 설치 → 부팅 확인
python prepare/make_firmware.py 1; python prepare/sign.py
Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3
python device/boot.py 3
```
| 실험 | 방법 | 결과 |
|---|---|---|
| 정상 부팅 (v1) | 위 마지막 줄 | |
| v2 설치 도중 전원 차단 | `python prepare/make_firmware.py 2; python prepare/sign.py` → `$env:POWER_CUT=524288; python device/main.py 3; Remove-Item Env:POWER_CUT` | |
| 전원 차단 직후 부팅 | `python device/boot.py 3` | |
| 다시 업데이트 | `python device/main.py 3` → `python device/boot.py 3` | |
| 한 번 더 업데이트 (v3) | `python prepare/make_firmware.py 3; python prepare/sign.py; python device/main.py 3; python device/boot.py 3` → 어느 칸에 써지나? | |

원상복구 : `Remove-Item Env:POWER_CUT -EA 0; python prepare/make_firmware.py 1; python prepare/sign.py; Remove-Item -Recurse -Force device/work`

생각할 것 : v2 가 **해시는 맞는데 실행하면 죽는** 펌웨어(버그)라면? boot.json 은 이미 B 를 가리킨다. A 로 돌아가려면?

**배운 점 / 남은 문제**
- (직접 기록)

---

## Step 14 : 시험 부팅 + 롤백

**왜** : A/B 는 "다 못 쓴" 펌웨어를 막는다. 하지만 해시·서명이 다 맞는데 **버그로 켜지자마자 죽는** 펌웨어는?
boot.json 이 이미 B 를 가리키니 B → 크래시 → 재부팅 → B → … A(v1) 는 멀쩡한데 벽돌.
검증은 "제작자가 만든 그대로인가" 만 확인한다. "실제로 잘 도는가" 는 **켜 봐야** 안다.

**방법** (MCUboot, ESP-IDF OTA 와 같은 방식)
```
설치 : boot.json = {slot: b, trial: true, prev: {slot: a, ...}}     "B 를 한 번 시험해 봐"
부팅 1 : trial → tried 기록 → B 실행
         ├ 잘 돎 → 확인 도장 → {slot: b} 로 확정
         └ 죽음 (실제 기기 : 워치독이 재부팅)
부팅 2 : tried 인데 도장 없음 → prev(A) 로 롤백
```
- Step 5 의 "롤백 방지" 와 반대 : 그건 공격자가 **옛 펌웨어를 밀어넣는 것**을 거부, 이건 기기가 **스스로 방금 전 잘 돌던 칸**으로 돌아가는 것

**구현**
- `device/install.py` : `save_boot()` (boot.json 한 번에 교체), 두 번째 설치부터 `trial` + `prev` 기록
- `device/boot.py` : 부트로더 흉내. `tried` 인데 도장 없으면 롤백 / `trial` 이면 `tried` 기록 후 실행 / 잘 켜지면 확인 도장
- `device/main.py` : 확인 전(`trial`)이면 업데이트 거부 (또 설치하면 돌아갈 칸 A 를 덮어쓰므로)
- `CRASH_VERSION=<버전>` : 그 버전 펌웨어는 부팅 때 죽음 (실험용)

**실험** (practice04 폴더, PowerShell, 서버 정상 모드)
```
# 준비 : v1 설치 → 부팅
python prepare/make_firmware.py 1; python prepare/sign.py
Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3; python device/boot.py 3

# 버그 있는 v2 배포 → 설치
python prepare/make_firmware.py 2; python prepare/sign.py
python device/main.py 3
```
| 실험 | 방법 | 결과 |
|---|---|---|
| 확인 전에 또 업데이트 | `python device/main.py 3` | |
| 부팅 1 (v2 가 죽음) | `$env:CRASH_VERSION=2; python device/boot.py 3` | |
| 부팅 2 | `python device/boot.py 3` (CRASH_VERSION 그대로) | |
| 부팅 3 | `python device/boot.py 3; Remove-Item Env:CRASH_VERSION` | |
| 롤백 후 다시 업데이트하면? | `python device/main.py 3` | **v2 를 다시 받아 설치** (boot.json 에는 "v1" 만 있고 v2 실패 기록이 없음) → 부팅하면 또 크래시 → 롤백 → 반복 |
| (비교) 정상 v3 | `python prepare/make_firmware.py 3; python prepare/sign.py` → 롤백 상태라면 main → boot → boot | |

`boot.json` 을 열어 보면서 하면 `trial`, `tried`, `prev` 가 어떻게 바뀌는지 보인다.

원상복구 : `Remove-Item Env:CRASH_VERSION -EA 0; python prepare/make_firmware.py 1; python prepare/sign.py; Remove-Item -Recurse -Force device/work`

생각할 것 : 롤백한 뒤 서버에는 여전히 v2 가 있다. 기기는 다시 v2 를 받을까? 그러면 어떻게 될까?

**배운 점 / 남은 문제**
- 시험 부팅 : 켜기 **전에** "시험했음" 을 기록해야 죽어도 다음 부팅 때 알 수 있다
- 롤백 자체는 되지만, 기기가 "v2 가 실패했다" 를 기억하지 못해 **v2 를 계속 다시 받는다**
  - 제작자가 고친 v3 를 내면 끝나지만, 그때까지 (며칠) 데이터·배터리·플래시 수명·서비스 중단·서버 부하 비용을 사람 없이 계속 치른다
- 해결 : 실패한 버전을 기록하고 건너뛴다 (Step 15). 실무에서는 서버에 실패를 **보고**해서 배포도 멈춘다

---

## Step 15 : 부팅 실패 버전 기록

**왜** : Step 14 에서 롤백 뒤 같은 v2 를 또 받아 크래시 → 롤백이 반복됐다.

**구현**
- `device/boot.py` : 롤백할 때 실패한 버전을 `bad_version.txt` 에 기록
- `device/download.py` : 버전 확인 다음에 `manifest 버전 <= 실패 버전` 이면 `[건너뜀]` (**청크 받기 전**에 멈춤 → 데이터 낭비 없음)
- `device/main.py` : `bad_version.txt` 읽어서 넘김

**실험** (practice04 폴더, PowerShell, 서버 정상 모드)
```
# 준비 : v1 설치 → 부팅 → 버그 있는 v2 설치
python prepare/make_firmware.py 1; python prepare/sign.py
Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3; python device/boot.py 3
python prepare/make_firmware.py 2; python prepare/sign.py
python device/main.py 3
```
| 실험 | 방법 | 결과 |
|---|---|---|
| v2 부팅 → 크래시 → 롤백 | `$env:CRASH_VERSION=2; python device/boot.py 3; python device/boot.py 3; Remove-Item Env:CRASH_VERSION` | |
| 다시 업데이트 (서버엔 여전히 v2) | `python device/main.py 3` | |
| 고친 v3 배포 | `python prepare/make_firmware.py 3; python prepare/sign.py; python device/main.py 3; python device/boot.py 3` | |

원상복구 : `Remove-Item Env:CRASH_VERSION -EA 0; python prepare/make_firmware.py 1; python prepare/sign.py; Remove-Item -Recurse -Force device/work`

**배운 점 / 남은 문제**
- (직접 기록)

---

## Step 16 : 이전 실습 대비 빠진 보안 항목 보완

**왜** : 09-23 (AES, CA, OpenSSL, split, local), 09-28 (practice01~03) 에서 했는데 practice04 에 없던 것

| # | 빠진 것 | 이전 실습 | 보완 |
|---|---|---|---|
| 1 | 인증서 **용도·주인** 확인 | practice02 : CN 확인 | 발급 때 `extendedKeyUsage=codeSigning` (펌웨어 서명 전용), 기기는 `openssl verify -purpose codesign` + 이름이 정확히 `CN=OTA Publisher, OU=key-N` 인지 |
| 2 | **부팅 때 서명 재검증** (보안 부팅) | practice03 `boot.py` | 설치할 때 칸 옆에 서명된 manifest·인증서 보관 → 부팅 때 CA → 인증서 → 서명 → hw → 펌웨어 해시 (boot.json 의 값은 안 믿음) |
| 3 | 펌웨어 **암호화** (기밀성) | 09-23 AES, CA, OpenSSL | AES-256-CTR. 서버에는 암호문(`firmware.enc`)만, 키는 개발자 PC + 기기(`device/trust/fw_key.hex`) |
| 4 | 응답 **구간(Content-Range)** 확인 | practice02·03 | 본문 받기 **전에** 요청 구간과 비교 → 다르면 재시도 (`full` 모드에서 1 MiB 를 받지 않음) |
| 5 | **manifest 구조** 확인 | practice02 | 크기 1 ~ 4 MiB, 청크 수 = 올림(크기 / 청크 크기) |

발견 경위 (1번) : 기존 인증서는 `openssl x509 -purpose` 로 보면 SSL server·S/MIME 등 **모든 용도 Yes**, Code signing 은 No 였다.
키 버전도 이름 전체에서 `key-숫자` 를 찾아서 `CN=monkey-99` 를 **99** 로 읽었다 → 그런 인증서 하나로 기기가 "최신 키 = 99" 를 기억하면 진짜 키를 영원히 거부 (업데이트 영구 차단)

**구현**
- `prepare/make_keys.py` : `openssl req -x509 -CA` 로 한 번에 발급, codeSigning 전용
- `prepare/make_fw_key.py` (새로) : 암호화 키 → `prepare/` + `device/trust/` (모든 기기 같은 키 : 한 대에서 털리면 전부 풀림)
- `prepare/make_firmware.py` : 평문 → 암호화 → `firmware.enc`. 청크·전체 해시는 **암호문**, `plain_sha256`·`iv` 추가. 평문은 `prepare/plain/`
- `device/verify.py` : `verify_cert(cert, now=None)` (용도 확인, now 없으면 시간 확인 안 함), `publisher_key_version()` (정확히 일치할 때만), `check_manifest()`
- `device/download.py` : Content-Range 확인, 인증서 이름, manifest 구조
- `device/install.py` : 암호문 확인 → 복호화 → 평문 확인 → 칸에 쓰기 → 서명된 manifest 보관
- `device/boot.py` : 보안 부팅
- `server/server.py` : 방해 대상 파일명 `firmware.enc`

**실험** (practice04 폴더, PowerShell, 서버 정상 모드. 준비 : `Remove-Item -Recurse -Force device/work -EA 0; python device/main.py 3; python device/boot.py 3`)

1) 다른 용도 인증서 (CA 가 서버용으로 발급한 인증서의 키가 털림)
```
python prepare/make_firmware.py 2
openssl genpkey -algorithm ed25519 -out other.key
openssl req -x509 -new -key other.key -subj "/CN=ota-server.local" -CA prepare/ca/ca.crt -CAkey prepare/ca/ca.key -days 365 -addext extendedKeyUsage=serverAuth -out other.crt
Copy-Item other.crt server/files/publisher.crt
openssl pkeyutl -sign -inkey other.key -rawin -in server/files/3/manifest.json -out server/files/3/manifest.json.sig
python device/main.py 3
```

2) 펌웨어 서명 용도지만 이름이 다름 (`CN=monkey-99`)
```
openssl req -x509 -new -key other.key -subj "/CN=monkey-99" -CA prepare/ca/ca.crt -CAkey prepare/ca/ca.key -days 365 -addext extendedKeyUsage=codeSigning -out other.crt
Copy-Item other.crt server/files/publisher.crt
openssl pkeyutl -sign -inkey other.key -rawin -in server/files/3/manifest.json -out server/files/3/manifest.json.sig
python device/main.py 3
```

3) 보안 부팅 (기기 저장소 변조) : 원상복구 후 v1 설치·부팅 상태에서
   - `device/work/3/slot_a.bin` 을 한 바이트 고치고 `python device/boot.py 3`
   - 이어서 `slot_a.manifest.json` 의 `plain_sha256` 도 고친 펌웨어의 해시로 바꾸고 다시 `python device/boot.py 3`

4) 암호화
   - `server/files/3/firmware.enc` 와 `prepare/plain/3.bin` 의 해시 비교 (서버에서 평문을 얻을 수 있나?)
   - `device/trust/fw_key.hex` 한 글자를 바꾸고 새 기기로 설치 (`device/work` 삭제 후 main)

5) Content-Range : `python server/server.py wrong` / `full` 로 켜고 새 기기로 설치 → 메시지 확인

6) manifest 구조 : `server/files/3/manifest.json` 의 `"size"` 를 `5000000` 으로 고치고 `python prepare/sign.py` (제작자가 실수로 서명) → 새 기기로 설치

| 실험 | 결과 |
|---|---|
| 1) 다른 용도 인증서 | |
| 2) 이름이 다른 인증서 | |
| 3) 칸 변조 / 칸 + 보관 manifest 변조 | |
| 4) 서버의 암호문 / 잘못된 키 | |
| 5) wrong / full | |
| 6) 이상한 size | |

원상복구 : `python prepare/make_firmware.py 1; python prepare/sign.py; Remove-Item other.key, other.crt -EA 0; Remove-Item -Recurse -Force device/work`
(4번에서 키를 바꿨다면 `Copy-Item prepare/fw_key.hex device/trust/fw_key.hex`)

**배운 점 / 남은 문제**
- (직접 기록)
