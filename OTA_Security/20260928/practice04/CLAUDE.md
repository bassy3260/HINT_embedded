# practice04 : 청크 OTA 직접 구현하며 보안 요소 검증하기

## 목적
학습한 OTA 보안 요소를 **처음부터 다시 구현**하면서, 각 요소가 **정말 필요한지, 왜 필요한지** 직접 확인한다.

## 과제
- 서버 : 서로 다른 랜덤 펌웨어 10종 (타겟 1~10)
- 기기 : 청크 단위로 받아 저장 → 설치
- 가정 : **네트워크 통신은 충분히 안전함** (MITM·도청은 범위 밖)
  → 그래도 남는 위협(서버 침해, 타겟 불일치, 다운그레이드, 저장소 변조, 전원 차단 등)을 하나씩 다룬다

## 진행 방식
1. 최소 구현 (보안 없음)
2. 공격 실험 → 무엇이 깨지는지 직접 확인
3. 왜 필요한지 정리 → 그 기능만 추가
4. 다시 실험해서 막히는지 확인

한 번에 한 단계만. 설명과 기능을 한꺼번에 몰아서 주지 않는다.

**공격·방해 실험은 학습자가 직접 한다.** 구현 쪽은 정상 동작만 확인하고,
실험은 `docs/EXPERIMENTS.md` 에 절차(명령)만 적어 결과 칸을 비워 둔다. 학습자가 요청할 때만 대신 실행한다.

### 역할 (서브에이전트, `OTA_Security/.claude/agents/`)
| 순서 | 에이전트 | 하는 일 |
|---|---|---|
| 1 | `ota-researcher` | 남은 위협 하나 + 공격 실험 제안 (코드 수정 X) |
| - | 학습자 | 실험해 보고 "왜 필요한지" 직접 생각 |
| 2 | `ota-coder` | 그 단계만 간단히 구현 → 실행 확인 → 문서 갱신 |
| 3 | `ota-reviewer` | 결과물만 보고 공격 재현·우회 시도로 검증 (코드 수정 X) |

## 폴더 구조 (어디서 일어나는 일인지로 나눔)
```
prepare/   [개발자 PC]  CA 생성(make_ca), 암호화 키(make_fw_key), 제작자 키·인증서(make_keys, 펌웨어 서명 전용), 펌웨어 생성·암호화(make_firmware), 서명·인증서 게시(sign)
server/    [배포]       server/files 의 암호화된 펌웨어·manifest 를 그대로 제공 (Range 지원). 판단 X, 비밀키 X, 청크로 자르지 않음. 실험용 방해 모드(인자)
device/    [기기]       download(인증서 용도·이름 → manifest 서명·구조 → Range 로 청크 요청 + 구간·해시 검증) → install(전체 검증 → 복호화 → A/B 칸에 쓰고 boot.json 커밋), boot.py(부트로더 흉내 : 서명부터 재검증하는 보안 부팅, 시험 부팅·롤백), verify.py 공용, main.py 가 순서대로 호출
```
- 새 기능은 역할에 맞는 파일로 추가 (예: `device/verify.py`, `prepare/sign.py`)
- 비밀키는 `prepare/ca/`, `prepare/keys/` 에만. 절대 `server/`·`device/` 에 두지 않는다 (기기는 `device/trust/ca.crt` 만 믿음)
- 펌웨어 암호화 키(`fw_key.hex`)는 `prepare/` 와 `device/trust/` 에만. 서버에는 암호문만, 평문 원본은 `prepare/plain/`
- 청크로 나누는 건 기기 쪽 일이다. 서버에는 펌웨어 원본 파일만 둔다 (실제 OTA 서버/CDN 과 같은 구조)

## 코드 스타일
- **진짜 간단하게.** 파일 하나 수십 줄 이내, 핵심 로직만
- 과한 예외처리·방어코드·옵션 X (실험으로 필요성이 드러난 것만 추가)
- 주석은 한국어로 짧게, 파일 맨 위에 `# [역할] 하는 일` 한 줄
- 표준 라이브러리 우선, 파일 입출력은 `pathlib`

## 실행 (practice04 폴더에서)
```
python prepare/make_ca.py        # 처음 한 번
python prepare/make_fw_key.py    # 처음 한 번
python prepare/make_keys.py 1    # 처음 한 번, 키 버전 (교체 때 prepare/keys 지우고 버전 올려 다시)
python prepare/make_firmware.py 1  # 버전 번호 (새로 배포할 때마다 올림), 유효기간 7일
python prepare/sign.py
python server/server.py        # 별도 터미널
python device/main.py 3        # 타겟 3 기기
```
산출물 : `server/files/<타겟>/` (firmware.enc, manifest.json, manifest.json.sig), `device/work/<타겟>/` (받은 인증서·manifest, 청크, slot_a/b.bin + 칸별 보관 manifest, boot.json, key_version.txt, last_time.txt, bad_version.txt)

## 문서 역할
| 문서 | 내용 | 언제 고치나 |
|---|---|---|
| `CLAUDE.md` | 프로젝트 구축 지침만 (목적, 가정, 구조, 스타일) | 규칙·구조가 바뀔 때 |
| `docs/MANUAL.md` | 켜기/끄기, 다운로드, 초기화, 문제 해결 | 실행 방법이 바뀔 때 |
| `docs/EXPERIMENTS.md` | 단계별 방어 내용, 실험 방법·결과, 남은 문제, 현재 방향 | 단계를 진행할 때마다 |
| `docs/흐름 한줄 정리.md` | 파트별(다운로드, manifest, CA …)로 한 일을 한 줄씩 | 방어를 추가할 때마다 한 줄 |

실험 기록·진행 상황은 CLAUDE.md 에 적지 않고 `docs/EXPERIMENTS.md` 에 적는다.
새 문서도 `docs/` 에 둔다 (CLAUDE.md 만 루트, 자동으로 읽히는 위치라서).
