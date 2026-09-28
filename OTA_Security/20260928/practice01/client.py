import os, sys, hmac, hashlib, http.client, urllib.request  # 필요한 기본 모듈들 불러오기

URL = 'http://192.168.0.6:9999/'  # 서버 주소
FILE = 'firmware.bin'  # 받을 펌웨어 파일
EXPECTED_SIZE = 104857600  # 기대 크기 (100 MiB)
EXPECTED_SHA256 = '5d6d4e12d1a9446768ccaf0ab8458b680ee3038be46b6168b2e51abf9613db64'  # 기대 해시 (서버에 해시 파일이 없어 미리 저장)
DEST = './work/device/' + FILE  # 최종 저장 위치
TMP = DEST + '.tmp'  # 검증 전까지는 임시 파일에 받음
CHUNK = 64 * 1024  # 100MB라 한 번에 메모리에 올리지 않고 64KB씩 받음

os.makedirs('./work/device', exist_ok=True)  # 저장할 폴더 만들기

# 1) 기대 해시 준비 (기본은 위에 저장한 값, python client.py <해시> 로 넘기면 그 값을 우선 사용)
expected_sha256 = (sys.argv[1] if len(sys.argv) > 1 else EXPECTED_SHA256).strip().lower()
print('기대 SHA-256 :', expected_sha256 or '(없음 → 계산만 하고 해시 검증은 생략)')

# 2) 이전에 받다 만 임시 파일이 있으면 그 내용까지 해시에 반영 (이어받기 준비)
h = hashlib.sha256()
size = 0
if os.path.exists(TMP):
    with open(TMP, 'rb') as f:
        for chunk in iter(lambda: f.read(CHUNK), b''):
            h.update(chunk)
            size += len(chunk)
    if size > EXPECTED_SIZE:  # 기대보다 크면 잘못된 파일 → 처음부터
        h, size = hashlib.sha256(), 0
        os.remove(TMP)
    print('이어받기 :', size, '바이트부터')

# 3) 스트리밍 다운로드 + 해시 동시 계산 (서버가 중간에 끊으면 Range 로 끊긴 지점부터 다시 요청)
MAX_RETRY = 50
retry = 0
with open(TMP, 'ab') as f:
    while size < EXPECTED_SIZE and retry < MAX_RETRY:
        req = urllib.request.Request(URL + FILE, headers={'Range': f'bytes={size}-'})
        before = size
        try:
            with urllib.request.urlopen(req, timeout=10) as r:
                if size and r.status != 206:  # Range 를 무시하고 처음부터 보내면 이어붙이면 안 됨
                    raise SystemExit('[실패] 서버가 이어받기(206)를 지원하지 않음')
                for chunk in iter(lambda: r.read(CHUNK), b''):
                    f.write(chunk)
                    h.update(chunk)
                    size += len(chunk)
                    print(f'\r다운로드 : {size}/{EXPECTED_SIZE} 바이트 ({size * 100 // EXPECTED_SIZE}%)', end='')
        except (OSError, http.client.HTTPException) as e:  # 연결 끊김/타임아웃 → 재시도
            print(f'\n[경고] 연결 오류 : {e}')
        if size < EXPECTED_SIZE:
            retry = 0 if size > before else retry + 1  # 진전이 있으면 재시도 횟수 초기화
            print(f'\n[재시도] {size} 바이트에서 끊김 → 이어받기')
print()
actual_sha256 = h.hexdigest()
print('계산 SHA-256 :', actual_sha256)

# 4) 검증 (크기 → 해시)
if size < EXPECTED_SIZE:  # 덜 받은 것 → 임시 파일은 남겨서 다음 실행 때 이어받기
    raise SystemExit(f'[실패] 다운로드 미완료 {size} / {EXPECTED_SIZE} → 다시 실행하면 이어받음')
if size != EXPECTED_SIZE:
    os.remove(TMP)
    raise SystemExit(f'[실패] 크기 불일치 {size} != {EXPECTED_SIZE} → 파일 폐기')

if not expected_sha256:
    print('[주의] 기대 해시가 없어 해시 검증을 건너뜀 → 위 계산값을 기록해 두세요')
elif not hmac.compare_digest(actual_sha256, expected_sha256):
    os.remove(TMP)
    raise SystemExit('[실패] 해시 불일치 → 파일 폐기')

# 5) 통과 시에만 교체 (원자적)
os.replace(TMP, DEST)
print('[성공] 검증 완료 :', size, '바이트 →', DEST)
