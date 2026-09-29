# OTA 펌웨어 분할 다운로더 (서명된 매니페스트 기반)
#  신뢰 사슬 : CA 지문(기기 저장) ─확인→ ca.crt ─검증→ publisher.crt ─공개키→ manifest 서명 ─통과→ 호환성·조각 해시·전체 해시 ─→ 설치
#  하나라도 틀리면 설치하지 않고 기존 펌웨어를 그대로 둔다
#  [축약] 표시가 붙은 주석 = 파이썬 축약 문법을 풀어 쓴 모양

import hashlib, http.client, os, subprocess, time, urllib.request
# [축약] 한 줄에 여러 개 import  →  import hashlib / import http.client / import os ... 를 한 줄씩 쓴 것과 같음

URL = 'http://192.168.0.6:9999/'
OPENSSL = r'C:\Program Files\Git\mingw64\bin\openssl.exe'
# [축약] r'...' = raw 문자열 : 역슬래시(\)를 특수문자로 해석하지 않음
#        안 쓰면 'C:\\Program Files\\Git\\...' 처럼 \ 를 두 번씩 써야 함
DEST = 'work/device/firmware.bin'
TMP = DEST + '.tmp'  # 검증이 끝나기 전까지는 임시 파일에 씀
RETRY = 5  # 조각 하나당 다시 받아 볼 횟수
EXPECTED_SHA256 = '5d6d4e12d1a9446768ccaf0ab8458b680ee3038be46b6168b2e51abf9613db64'  # 교수님이 공식 해시로 확인해 준 값 (manifest version 6)

# 신뢰 앵커 : Root CA 의 지문(SHA-256)을 기기에 넣어 둠
#   ca.crt 는 서버에서 받지만, 지문이 이 값과 같을 때만 믿음 (다르면 CA 바꿔치기 → 거부)
#   서버 CA 가 바뀌면 교수님께 새 지문을 확인받고 이 한 줄만 바꾸면 됨
#   지문 기록 : 93:97:FE:70:… (~13시) → 68:1C:32:D5:… (14시경) → 99:61:00:14:… (현재)
CA_FINGERPRINT = '99:61:00:14:74:45:D9:5C:C1:CC:E8:29:21:2D:14:0D:FC:E8:B5:67:9E:9D:F3:41:62:2F:6E:E6:06:02:82:81'

# 이 기기(유닛) 정보 : manifest 의 target 과 같아야 설치 (다른 유닛/하드웨어용 펌웨어 설치 방지)
DEVICE_ECU = 'brake-ecu'  # 유닛 종류
DEVICE_HW = 'hw3'  # 하드웨어 리비전


def get(name, start=None, end=None):  # 서버에서 파일(또는 start~end 범위만) 받기
    # [축약] start=None : 기본값 있는 인자 → get('manifest.txt') 처럼 start, end 를 안 넘기면 None
    headers = {'Range': f'bytes={start}-{end}'} if start is not None else {}
    # [축약] 조건 표현식 (A if 조건 else B) + f-string 풀어 쓰면:
    #   if start is not None:
    #       headers = {'Range': 'bytes=' + str(start) + '-' + str(end)}
    #   else:
    #       headers = {}
    with urllib.request.urlopen(urllib.request.Request(URL + name, headers=headers), timeout=10) as r:
        # [축약] with ... as r : 블록이 끝나면(에러가 나도) 연결을 자동으로 닫아 줌. 풀어 쓰면:
        #   r = urllib.request.urlopen(...)
        #   try:
        #       ...
        #   finally:
        #       r.close()
        return r.headers.get('Content-Range'), r.read()
        # [축약] 값 두 개를 콤마로 반환 = 튜플 (Content-Range, 데이터) 하나를 반환하는 것
        #   받는 쪽 :  cr, data = get(...)      → 두 변수에 나눠 담기 (언패킹)
        #             get(...)[1]              → 두 번째 값(데이터)만 꺼내기


def ssl(*args):  # openssl 실행 → 결과 (returncode 0 이면 성공)
    # [축약] *args : 넘긴 인자를 몇 개든 튜플로 모아 받음  ssl('verify', '-CAfile', ...) → args = ('verify', '-CAfile', ...)
    return subprocess.run([OPENSSL, *args], capture_output=True, text=True)
    # [축약] [OPENSSL, *args] : 리스트 안에서 *args 를 펼침. 풀어 쓰면:
    #   cmd = [OPENSSL]
    #   for a in args:
    #       cmd.append(a)


def discard_tmp():  # 검증 실패한 임시 파일 버리기
    # 방금 만든 큰 파일은 백신(Windows Defender 등)이 잠깐 잡고 있어서 바로 안 지워질 수 있음 → 몇 번 기다렸다 재시도
    for _ in range(10):
        try:
            os.remove(TMP)
            return
        except FileNotFoundError:  # 이미 없으면 끝
            return
        except PermissionError:  # 다른 프로그램이 사용 중 → 0.5초 뒤 다시
            time.sleep(0.5)
    print('[주의] 임시 파일을 지우지 못함 (설치는 안 됨) :', TMP)


# 1) manifest / 서명 / Publisher 인증서 / Root CA 받기 (받은 그대로 저장 → 직접 열어볼 수 있음)
for name in ('manifest.txt', 'manifest.txt.sig', 'publisher.crt', 'ca.crt'):
    open(name, 'wb').write(get(name)[1])
    # [축약] 파일 열기 + 쓰기를 한 줄로. 풀어 쓰면:
    #   content_range, data = get(name)
    #   f = open(name, 'wb')      # w = 쓰기(덮어쓰기), b = 바이너리
    #   f.write(data)
    #   f.close()                 # 한 줄로 쓰면 close 를 안 부르지만, 파이썬이 곧바로 알아서 닫아 줌

# 2) 서명 검증 : 하나라도 실패하면 위조 의심 → 아무것도 받지 않고 종료
fingerprint = ssl('x509', '-in', 'ca.crt', '-noout', '-fingerprint', '-sha256').stdout.split('=')[-1].strip()
# 출력 예: 'sha256 Fingerprint=99:61:00:...'  → '=' 로 잘라 마지막 부분([-1])만, 앞뒤 공백/줄바꿈 제거(strip)
if fingerprint != CA_FINGERPRINT:
    raise SystemExit(f'[위조 의심] 서버 CA 지문이 기기에 저장된 값과 다름 : {fingerprint}')
    # raise SystemExit('메시지') : 메시지 출력 후 프로그램 종료 → 아래 코드는 실행되지 않음
if ssl('verify', '-CAfile', 'ca.crt', 'publisher.crt').returncode != 0:
    raise SystemExit('[위조 의심] publisher.crt 가 Root CA 로 검증되지 않음')
if 'CN=PKNU OTA Publisher' not in ssl('x509', '-in', 'publisher.crt', '-noout', '-subject').stdout.replace(' = ', '='):
    raise SystemExit('[위조 의심] 인증서 주인이 PKNU OTA Publisher 가 아님')
    # [축약] 메서드 체인 + not in. 풀어 쓰면:
    #   result = ssl('x509', '-in', 'publisher.crt', '-noout', '-subject')
    #   subject = result.stdout                    # 예: 'subject=C=KR, O=PKNU OTA Lab, CN=PKNU OTA Publisher'
    #   subject = subject.replace(' = ', '=')      # openssl 버전에 따라 'CN = ...' 로 나와서 통일
    #   if subject.find('CN=PKNU OTA Publisher') == -1:   # 문자열이 안 들어 있으면
    #       raise SystemExit(...)
ssl('x509', '-in', 'publisher.crt', '-noout', '-pubkey', '-out', 'publisher_pub.pem')  # 검증된 인증서에서 공개키 꺼내기
if ssl('pkeyutl', '-verify', '-pubin', '-inkey', 'publisher_pub.pem', '-rawin',
       '-in', 'manifest.txt', '-sigfile', 'manifest.txt.sig').returncode != 0:
    # 괄호 안에서는 줄을 바꿔도 한 줄로 이어짐
    raise SystemExit('[위조 의심] manifest 서명 검증 실패')
print('서명 검증 통과 : CA 지문 → publisher.crt → manifest')

# 3) manifest 읽기 : "이름 값…" 줄(version / expires / target / size / piece / count) + "조각번호 해시" count 줄
rows = [line.split() for line in open('manifest.txt') if line.strip()]
# [축약] 리스트 컴프리헨션. 풀어 쓰면:
#   rows = []
#   for line in open('manifest.txt'):          # 파일을 한 줄씩 읽음
#       if line.strip() != '':                 # 빈 줄은 건너뜀 (strip = 앞뒤 공백/줄바꿈 제거)
#           rows.append(line.split())          # 'size 104857600\n' → ['size', '104857600']
meta = {row[0]: ' '.join(row[1:]) for row in rows if not row[0].isdigit()}
# [축약] 딕셔너리 컴프리헨션 + 조건. 풀어 쓰면:
#   meta = {}
#   for row in rows:
#       if not row[0].isdigit():               # 숫자로 시작하는 줄(조각 해시)은 빼고
#           meta[row[0]] = ' '.join(row[1:])   # 두 번째 값부터 끝까지 공백으로 이어 붙임
#                                              # ['target', 'brake-ecu', 'hw3'] → meta['target'] = 'brake-ecu hw3'
#   결과 : {'version': '6', 'expires': '2027-...', 'target': 'brake-ecu hw3', 'size': '104857600', ...}
print('manifest 정보 :', meta)

# 호환성 확인 : 이 기기(유닛 + 하드웨어 리비전)용 펌웨어가 아니면 정품이어도 설치하면 안 됨
if meta.get('target') != f'{DEVICE_ECU} {DEVICE_HW}':
    # meta.get('target') : target 줄이 없으면 에러 대신 None → 확인할 수 없으므로 역시 거부
    raise SystemExit(f"[호환성 불일치] 대상 {meta.get('target')} ≠ 이 기기 {DEVICE_ECU} {DEVICE_HW} → 설치 거부")
print('호환성 확인 :', DEVICE_ECU, DEVICE_HW)
size, piece, count = int(meta['size']), int(meta['piece']), int(meta['count'])
# [축약] 여러 변수에 한 번에 대입. 풀어 쓰면:
#   size = int(meta['size'])                   # 문자열 → 숫자
#   piece = int(meta['piece'])
#   count = int(meta['count'])
hashes = {int(row[0]): row[1] for row in rows if row[0].isdigit()}
# [축약] 딕셔너리 컴프리헨션 + 조건. 풀어 쓰면:
#   hashes = {}
#   for row in rows:
#       if row[0].isdigit():                   # 숫자로 시작하는 줄만
#           hashes[int(row[0])] = row[1]       # {0: 'ea40...', 1: '...', ..., 99: '...'}
if sorted(hashes) != list(range(count)) or not piece * (count - 1) < size <= piece * count:
    raise SystemExit('[실패] manifest 형식 오류')
    # [축약] 풀어 쓰면:
    #   keys = sorted(hashes.keys())           # 딕셔너리를 sorted 하면 키만 정렬된 리스트 [0, 1, ..., 99]
    #   if keys != [0, 1, 2, ..., count-1]:    # 조각 번호가 빠지거나 중복 없이 0 ~ count-1 인지
    #       raise SystemExit(...)
    #   if not (piece * (count - 1) < size and size <= piece * count):   # 연쇄 비교 a < b <= c
    #       raise SystemExit(...)              # 조각 count 개로 크기를 딱 덮는지 (마지막 조각은 작을 수 있음)

# 4) 조각을 0번부터 순서대로 받으면서 하나씩 검증 → 통과한 조각만 임시 파일에 이어 씀
os.makedirs('work/device', exist_ok=True)  # 폴더 만들기 (이미 있으면 그냥 넘어감)
whole = hashlib.sha256()  # 전체 파일 해시 : 조각을 쓸 때마다 이어서 계산 (다 쓴 뒤 파일을 다시 읽지 않아도 됨)
with open(TMP, 'wb') as out:  # 블록이 끝나면 파일 자동으로 닫힘
    for i in range(count):  # i = 0, 1, 2, ..., 99
        start, end = i * piece, min((i + 1) * piece, size) - 1
        # [축약] 풀어 쓰면:
        #   start = i * piece                                  # 조각 시작 위치
        #   end = (i + 1) * piece - 1                          # 조각 끝 위치 (포함)
        #   if end > size - 1:                                 # 마지막 조각이 파일 끝을 넘으면
        #       end = size - 1                                 # 파일 끝까지만
        for attempt in range(RETRY):  # 최대 5번 시도
            try:
                content_range, data = get('firmware.bin', start, end)
            except (OSError, http.client.HTTPException):  # 연결 끊김/타임아웃 → 다시
                continue  # continue : 이번 시도는 버리고 다음 시도로
            if content_range != f'bytes {start}-{end}/{size}':  # 요청한 위치·크기의 데이터가 맞는지
                continue
            if hashlib.sha256(data).hexdigest() == hashes[i]:  # i번 자리 조각이 manifest 의 i번 해시와 같은지
                break  # break : 검증 통과 → 시도 반복을 빠져나감 (아래 else 는 실행 안 됨)
        else:  # RETRY 번 모두 실패 → 설치 거부
            # [축약] for ... else : for 가 break 없이 끝까지 돌았을 때만 else 실행. 풀어 쓰면:
            #   ok = False
            #   for attempt in range(RETRY):
            #       ...
            #       if 해시 일치:
            #           ok = True
            #           break
            #   if not ok:
            #       (아래 코드)
            out.close()
            discard_tmp()
            raise SystemExit(f'\n[변조 탐지] 조각 {i} ({start}~{end}) 검증 실패 → 설치 거부')
        out.write(data)  # 검증 통과한 조각만 이어 씀 (0번부터 순서대로라 순서가 꼬일 수 없음)
        whole.update(data)  # 쓴 순서 그대로 전체 해시에도 넣음
        print(f'\r조각 {i + 1}/{count} 검증 완료', end='')
        # [축약] f-string : {} 안의 변수 값이 들어감  → '\r조각 ' + str(i + 1) + '/' + str(count) + ' 검증 완료'
        #        \r : 줄 맨 앞으로 돌아가 같은 줄에 덮어쓰기 / end='' : print 끝에 줄바꿈을 붙이지 않음
print()  # 진행률 줄 끝내기 (줄바꿈)

# 5) 전체 해시 확인 : 조각이 모두 맞아도, 최종 파일이 교수님이 알려준 펌웨어와 같은지 한 번 더 확인
print('기대 SHA-256 :', EXPECTED_SHA256)
print('계산 SHA-256 :', whole.hexdigest())
if whole.hexdigest() != EXPECTED_SHA256:
    discard_tmp()  # 다른 펌웨어 → 설치하지 않고 버림 (기존 설치본은 그대로)
    raise SystemExit('[실패] 전체 해시 불일치 → 설치 거부')

# 6) 모든 검증을 통과했을 때만 설치 (os.replace : 한 번에 교체 → 반쯤 덮어쓴 파일이 생기지 않음)
os.replace(TMP, DEST)
print('[성공] 설치 완료 :', size, '바이트 →', DEST)
