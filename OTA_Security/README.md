# OTA_Security

OTA 실습. 폴더마다 기능이 하나씩 늘어나요.

| 폴더 | 서버 | 클라이언트 |
|---|---|---|
| `local` | A.bin 해시 → manifest.json | 받기 → 크기·해시 검증 → 저장 |
| `AES` | A.bin AES 암호화 (키 고정) + 해시 | 받기 → 해시 검증 → 복호화 → 평문 해시 검증 |
| `OpenSSL` | RSA 키 생성 → AES 암호화 → AES 키를 개인키로 암호화 → 해시 | 받기 → 해시 검증 → 공개키로 AES 키 복원 → 복호화 |
| `CA` | OpenSSL + 해시 서명(`.sig`) | 받기 → **서명 검증** → 해시 검증 → 키 복원 → 복호화 |
| `split` | 100MB 파일 → 64KB 조각마다 해시 + 서명 | 조각마다 서명 검증 → 해시 검증 → 합치기 → 원본 비교 |

- `key/`: XCA 인증서·키 (`split`에서 사용)
- `demo/`: AES, SHA-256 연습 코드
- 실행: 폴더 안에서 `python server.py` → `python client.py`
- `OpenSSL/client.py`는 다른 PC 서버(`192.168.0.19:9999`, `firmware.*`)에 맞춰져 있음
