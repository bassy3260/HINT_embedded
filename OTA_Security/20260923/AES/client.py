import os, json, hmac, hashlib, urllib.request   # os: 파일/폴더 작업, json: manifest 해석, hmac: 안전한 비교, hashlib: SHA-256, urllib: HTTP 다운로드
from Crypto.Cipher import AES                     # AES 암호 알고리즘 (pycryptodome 라이브러리)
from Crypto.Util.Padding import unpad             # 복호화 후 붙어있는 패딩(채움 바이트) 제거용

# ───────────── 설정값 ─────────────
KEY = bytes.fromhex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")  # 32바이트 = AES-256
  # AES-128 키 16바이트. 서버와 똑같은 키여야 복호화 가능 (기기에 미리 심어둔 값)
URL = 'http://192.168.0.26:8000'   # 서버 주소 (server.py 가 켜져 있는 PC의 IP:포트)
DEST = './work/device/A.bin'       # 최종적으로 저장될 복호화된 파일 위치
TMP_ENC = DEST + '.enc.tmp'        # 다운받은 암호문을 임시로 저장할 파일 (./work/device/A.bin.enc.tmp)
TMP_DEC = DEST + '.tmp'            # 복호화된 평문을 임시로 저장할 파일 (./work/device/A.bin.tmp)
os.makedirs('./work/device', exist_ok=True)  # 저장 폴더가 없으면 만들기 (이미 있으면 그냥 넘어감)

# ───────────── 0) manifest 받기 ─────────────
# 서버의 manifest.json 을 받아서 파이썬 딕셔너리로 변환
# 내용 예: {"file": "A.enc", "size": 1040, "sha256": "암호문해시", "plain_sha256": "평문해시"}
manifest = json.loads(urllib.request.urlopen(URL + '/manifest.json').read())

# ───────────── 1) 암호문 다운로드 + 해시 계산 ─────────────
h = hashlib.sha256()   # SHA-256 계산기 생성 (데이터를 조금씩 넣으면 누적해서 계산함)
size = 0               # 받은 총 바이트 수를 셀 변수
# URL/A.enc 에 접속(r) 하면서 동시에 임시파일(f)을 쓰기 모드로 열기
with urllib.request.urlopen(URL + '/' + manifest['file']) as r, open(TMP_ENC, 'wb') as f:
    # 4096바이트씩 끊어서 읽기. 더 읽을 게 없으면(b'' 빈 값) 반복 종료
    for chunk in iter(lambda: r.read(4096), b''):
        f.write(chunk)         # 받은 조각을 임시파일에 기록
        h.update(chunk)        # 같은 조각을 해시 계산기에도 넣기
        size += len(chunk)     # 받은 크기 누적
# with 블록이 끝나면 연결과 파일이 자동으로 닫힘

# ───────────── 2) 암호문 검증 ─────────────
# 받은 크기 != 서버가 알려준 크기  또는  계산한 해시 != 서버가 알려준 해시  → 실패
# (hmac.compare_digest = 문자열 비교인데 걸리는 시간이 항상 같아서 타이밍 공격에 안전)
if size != manifest['size'] or not hmac.compare_digest(h.hexdigest(), manifest['sha256']):
    os.remove(TMP_ENC)                           # 깨졌거나 변조된 파일이므로 삭제
    raise SystemExit('[실패] 암호문 검증 실패 → 폐기')  # 프로그램 종료 (복호화 단계로 안 넘어감)
print('[OK] 암호문 해시 일치')

# ───────────── 3) 복호화 ─────────────
data = open(TMP_ENC, 'rb').read()   # 검증된 암호문 파일을 통째로 읽기 (바이트)
os.remove(TMP_ENC)                   # 메모리에 읽었으니 임시 암호문 파일은 삭제
iv, ct = data[:16], data[16:]        # 서버가 [IV 16바이트][암호문] 순서로 붙여 보냈으므로 앞 16바이트=IV, 나머지=암호문
try:
    # AES.new(키, CBC모드, IV) 로 복호화기 생성 → .decrypt(암호문) 으로 복호화
    # → 결과 끝에 패딩이 붙어 있으므로 unpad 로 제거 → 원래 평문
    plain = unpad(AES.new(KEY, AES.MODE_CBC, iv).decrypt(ct), AES.block_size)
except ValueError:
    # 키가 틀리면 복호화 결과가 쓰레기값이 되어 패딩 형식이 안 맞음 → unpad 가 ValueError 발생
    raise SystemExit('[실패] 복호화 실패 (키 불일치/패딩 오류)')

# ───────────── 4) 평문 검증 ─────────────
# 복호화한 평문의 해시 == 서버가 알려준 원본(A.bin) 해시 인지 확인 → 복호화가 정확히 됐는지 최종 확인
if not hmac.compare_digest(hashlib.sha256(plain).hexdigest(), manifest['plain_sha256']):
    raise SystemExit('[실패] 평문 해시 불일치')
print('[OK] 평문 해시 일치')

# ───────────── 5) 적용 ─────────────
open(TMP_DEC, 'wb').write(plain)   # 평문을 먼저 임시파일에 저장
os.replace(TMP_DEC, DEST)          # 임시파일 → 최종파일로 한 번에 교체 (중간에 꺼져도 반쯤 쓴 파일이 안 생김)
print('[성공]', len(plain), '바이트 →', DEST)
