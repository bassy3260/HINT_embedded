# OTA 펌웨어 다운로더
#  - 서버에서 100MB 펌웨어를 받는다 (서버가 중간에 끊어도 끊긴 곳부터 이어받기)
#  - 크기 + SHA-256 해시가 맞을 때만 설치(저장)하고, 틀리면 버린다

import os, sys, hmac, hashlib, http.client, urllib.request  # 필요한 기본 모듈들 불러오기
# os          : 파일/폴더 다루기 (만들기, 지우기, 이름 바꾸기)
# sys         : 실행할 때 넘긴 인자(sys.argv) 읽기
# hmac        : 해시 두 개를 안전하게 비교 (compare_digest)
# hashlib     : SHA-256 해시 계산
# http.client : 다운로드 중 연결이 깨졌을 때 나는 예외 종류
# urllib      : HTTP 로 서버에서 파일 받기

URL = 'http://192.168.0.6:9999/'  # 서버 주소
FILE = 'firmware.bin'  # 받을 펌웨어 파일
EXPECTED_SIZE = 104857600  # 기대 크기 (100 MiB = 100 * 1024 * 1024)
EXPECTED_SHA256 = '5d6d4e12d1a9446768ccaf0ab8458b680ee3038be46b6168b2e51abf9613db64'  # 기대 해시 (서버에 해시 파일이 없어 미리 저장)
DEST = './work/device/' + FILE  # 최종 저장 위치 (= 기기에 설치된 펌웨어라고 생각)
TMP = DEST + '.tmp'  # 검증 전까지는 임시 파일에 받음 (검증 실패해도 기존 펌웨어는 안전)
CHUNK = 64 * 1024  # 100MB라 한 번에 메모리에 올리지 않고 64KB씩 받음

os.makedirs('./work/device', exist_ok=True)  # 저장할 폴더 만들기 (이미 있으면 그냥 넘어감)

# 1) 기대 해시 준비 (기본은 위에 저장한 값, python client.py <해시> 로 넘기면 그 값을 우선 사용)
#    sys.argv[0] 은 파일 이름(client.py), sys.argv[1] 이 첫 번째 인자
#    .strip() 앞뒤 공백 제거, .lower() 소문자로 통일 (hexdigest 결과가 소문자라서)
expected_sha256 = (sys.argv[1] if len(sys.argv) > 1 else EXPECTED_SHA256).strip().lower()
print('기대 SHA-256 :', expected_sha256 or '(없음 → 계산만 하고 해시 검증은 생략)')  # 빈 문자열이면 or 뒤 문구 출력

# 2) 이전에 받다 만 임시 파일이 있으면 그 내용까지 해시에 반영 (이어받기 준비)
h = hashlib.sha256()  # 해시 계산기 (update 로 데이터를 조금씩 넣으면 누적 계산)
size = 0  # 지금까지 받은 바이트 수 = 다음에 서버에 요청할 시작 위치
if os.path.exists(TMP):  # 지난번 실행이 중간에 멈춰서 .tmp 가 남아 있으면
    with open(TMP, 'rb') as f:  # 읽기(r) + 바이너리(b) 모드로 열기
        for chunk in iter(lambda: f.read(CHUNK), b''):  # 64KB씩 읽기, 빈 값(b'')이 나오면 파일 끝 → 반복 종료
            h.update(chunk)  # 이미 받은 부분도 해시에 넣어야 최종 해시가 전체 파일 기준이 됨
            size += len(chunk)
    if size > EXPECTED_SIZE:  # 기대보다 크면 잘못된 파일 → 처음부터
        h, size = hashlib.sha256(), 0  # 해시 계산기와 크기 초기화
        os.remove(TMP)
    print('이어받기 :', size, '바이트부터')

# 3) 스트리밍 다운로드 + 해시 동시 계산 (서버가 중간에 끊으면 Range 로 끊긴 지점부터 다시 요청)
MAX_RETRY = 50  # 한 바이트도 못 받은 채로 50번 연속 실패하면 포기 (무한 반복 방지)
retry = 0
with open(TMP, 'ab') as f:  # 추가(a) 모드 → 기존 내용 뒤에 이어서 씀
    while size < EXPECTED_SIZE and retry < MAX_RETRY:  # 다 받을 때까지 계속 요청
        # Range 헤더 : "size 번째 바이트부터 끝까지 보내줘" (예: bytes=21037056-)
        req = urllib.request.Request(URL + FILE, headers={'Range': f'bytes={size}-'})
        before = size  # 이번 요청에서 진전이 있었는지 비교하려고 기록
        try:
            with urllib.request.urlopen(req, timeout=10) as r:  # 10초 동안 응답 없으면 타임아웃 예외
                # 200 = 파일 전체를 처음부터 보냄, 206 = 요청한 부분만 보냄(Partial Content)
                if size and r.status != 206:  # 이어받는 중인데 처음부터 보내면 이어붙이면 파일이 망가짐
                    raise SystemExit('[실패] 서버가 이어받기(206)를 지원하지 않음')  # 프로그램 종료
                for chunk in iter(lambda: r.read(CHUNK), b''):  # 서버가 연결을 끊으면 b'' 가 와서 반복 종료
                    f.write(chunk)  # 파일에 쓰고
                    h.update(chunk)  # 해시에 넣고
                    size += len(chunk)  # 받은 크기 누적
                    # \r : 줄 맨 앞으로 돌아가서 같은 줄에 덮어쓰기 → 진행률이 한 줄에서 갱신됨
                    print(f'\r다운로드 : {size}/{EXPECTED_SIZE} 바이트 ({size * 100 // EXPECTED_SIZE}%)', end='')
        except (OSError, http.client.HTTPException) as e:  # 연결 끊김/타임아웃 → 프로그램을 죽이지 않고 잡아서 재시도
            print(f'\n[경고] 연결 오류 : {e}')
        if size < EXPECTED_SIZE:  # 아직 덜 받았으면 (서버가 중간에 끊은 것)
            retry = 0 if size > before else retry + 1  # 조금이라도 받았으면 재시도 횟수 초기화, 못 받았으면 +1
            print(f'\n[재시도] {size} 바이트에서 끊김 → 이어받기')
print()  # 진행률 줄 끝내기 (줄바꿈)
actual_sha256 = h.hexdigest()  # 누적 계산한 해시를 16진수 문자열(64글자)로
print('계산 SHA-256 :', actual_sha256)

# 4) 검증 (크기 → 해시)  raise SystemExit : 메시지를 출력하고 여기서 프로그램 종료 → 아래 설치 단계로 못 감
if size < EXPECTED_SIZE:  # 덜 받은 것 → 임시 파일은 남겨서 다음 실행 때 이어받기
    raise SystemExit(f'[실패] 다운로드 미완료 {size} / {EXPECTED_SIZE} → 다시 실행하면 이어받음')
if size != EXPECTED_SIZE:  # 더 많이 받은 것 → 잘못된 파일이므로 폐기
    os.remove(TMP)
    raise SystemExit(f'[실패] 크기 불일치 {size} != {EXPECTED_SIZE} → 파일 폐기')

if not expected_sha256:  # 기대 해시를 비워 두면 비교할 기준이 없음
    print('[주의] 기대 해시가 없어 해시 검증을 건너뜀 → 위 계산값을 기록해 두세요')
elif not hmac.compare_digest(actual_sha256, expected_sha256):  # == 대신 사용 : 비교 시간이 일정해서 타이밍 공격에 안전
    os.remove(TMP)  # 깨졌거나 변조된 파일 → 폐기
    raise SystemExit('[실패] 해시 불일치 → 파일 폐기')

# 5) 통과 시에만 교체 (원자적)
#    os.replace 는 이름만 바꾸는 한 번의 동작이라 중간 상태가 없음
#    → 기존 펌웨어 or 새 펌웨어 둘 중 하나만 존재 (반쯤 덮어쓴 파일이 생기지 않음)
os.replace(TMP, DEST)
print('[성공] 검증 완료 :', size, '바이트 →', DEST)
