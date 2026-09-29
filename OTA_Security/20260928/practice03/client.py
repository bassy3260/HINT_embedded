# OTA 제자리(in-place) 업데이트 : 임시 파일 없이, 저장 공간에 펌웨어 한 벌만 있다고 가정
#  1) manifest 검증  2) manifest 를 펌웨어 옆에 보관 + 사용 가능 플래그(valid) 끄기
#  3) 펌웨어의 조각을 새 manifest 해시와 비교 → 틀린 조각만 병렬로 받아 제자리에 덮어쓰기
#  4) 전체 재검증 후 valid 켜기
#  - 진행 기록 파일 없음 : 끊긴 뒤 다시 실행하면 3)에서 해시가 맞는 조각은 건너뜀 (해시가 곧 진행 기록)
#  - valid 가 꺼진 동안은 boot.py(부팅 검사)가 펌웨어 실행을 거부 → 반쯤 쓴 펌웨어는 절대 실행되지 않음
#  - 로그 : 화면에는 단계별 결과(INFO), work/ota.log 에는 조각별 상세 기록(DEBUG)

import datetime, hashlib, http.client, logging, os, shutil, subprocess, time, urllib.request
from concurrent.futures import ThreadPoolExecutor

URL = 'http://192.168.0.6:9999/'
OPENSSL = r'C:\Program Files\Git\mingw64\bin\openssl.exe'
CA_FINGERPRINT = '99:61:00:14:74:45:D9:5C:C1:CC:E8:29:21:2D:14:0D:FC:E8:B5:67:9E:9D:F3:41:62:2F:6E:E6:06:02:82:81'
DEVICE = 'brake-ecu hw3'  # 이 기기(유닛 + 하드웨어)
DIR = 'work/device/'  # 기기 저장소
FIRMWARE = DIR + 'firmware.bin'  # 펌웨어 (한 벌만)
VALID = DIR + 'valid'  # 사용 가능 플래그 파일 (있으면 사용 가능)
SIGNED = ['manifest.txt', 'manifest.txt.sig', 'publisher.crt', 'ca.crt']  # 펌웨어와 함께 보관할 파일
LOG_FILE = 'work/ota.log'
WORKERS = 8
RETRY = 5

# 로그 설정 : 파일에는 DEBUG 까지 전부, 화면에는 INFO 이상만
os.makedirs(DIR, exist_ok=True)
log = logging.getLogger('ota')
log.setLevel(logging.DEBUG)
file_handler = logging.FileHandler(LOG_FILE, encoding='utf-8')  # 기본 모드 'a' : 실행할 때마다 이어서 기록
file_handler.setFormatter(logging.Formatter('%(asctime)s [%(levelname)-5s] [%(threadName)-12s] %(message)s'))
screen_handler = logging.StreamHandler()
screen_handler.setLevel(logging.INFO)
screen_handler.setFormatter(logging.Formatter('%(asctime)s [%(levelname)s] %(message)s', '%H:%M:%S'))
log.addHandler(file_handler)
log.addHandler(screen_handler)


def fail(message):  # 실패 : 로그에 남기고 종료
    log.error(message)
    log.info('===== 업데이트 종료 (실패) =====')
    raise SystemExit(1)


def get(name, start=None, end=None):  # 서버에서 파일 받기 (start, end 를 주면 그 범위만)
    headers = {'Range': f'bytes={start}-{end}'} if start is not None else {}
    with urllib.request.urlopen(urllib.request.Request(URL + name, headers=headers), timeout=10) as r:
        return r.headers.get('Content-Range'), r.read()


def openssl(*args):
    return subprocess.run([OPENSSL, *args], capture_output=True, text=True)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def read_manifest(path):  # manifest → (info, hashes)
    info, hashes = {}, {}
    for line in open(path):
        key, *values = line.split()  # 'target brake-ecu hw3' → key='target', values=['brake-ecu', 'hw3']
        if key.isdigit():
            hashes[int(key)] = values[0]
        else:
            info[key] = ' '.join(values)
    return info, hashes


log.info('===== 업데이트 시작 =====')
log.debug(f'서버 {URL} / 기기 {DEVICE} / 병렬 {WORKERS} / 조각당 시도 {RETRY}회')
started = time.time()

# 1) 새 manifest 받아서 검증 (작업 폴더 work/ 에 받음 : 몇 KB 라 공간 문제 없음)
for name in SIGNED:
    try:
        data = get(name)[1]
    except (OSError, http.client.HTTPException) as e:
        fail(f'[네트워크] {name} 받기 실패 : {e!r}')
    open('work/' + name, 'wb').write(data)
    log.debug(f'받음 {name} : {len(data)} 바이트, sha256 {sha256(data)[:16]}…')
os.chdir('work')  # openssl 명령을 짧게 쓰려고 work/ 에서 실행
fingerprint = openssl('x509', '-in', 'ca.crt', '-noout', '-fingerprint', '-sha256').stdout.split('=')[-1].strip()
log.debug(f'CA 지문 : 서버 {fingerprint} / 기기 저장값 {CA_FINGERPRINT}')
if fingerprint != CA_FINGERPRINT:
    fail('[위조 의심] CA 지문 불일치')
result = openssl('verify', '-CAfile', 'ca.crt', 'publisher.crt')
log.debug(f'인증서 검증 : {(result.stdout + result.stderr).strip()}')
if result.returncode != 0:
    fail('[위조 의심] publisher.crt 가 CA 로 검증되지 않음')
log.debug('인증서 주인 : ' + openssl('x509', '-in', 'publisher.crt', '-noout', '-subject').stdout.strip())
openssl('x509', '-in', 'publisher.crt', '-noout', '-pubkey', '-out', 'publisher_pub.pem')
result = openssl('pkeyutl', '-verify', '-pubin', '-inkey', 'publisher_pub.pem', '-rawin',
                 '-in', 'manifest.txt', '-sigfile', 'manifest.txt.sig')
log.debug(f'manifest 서명 검증 : {(result.stdout + result.stderr).strip()}')
if result.returncode != 0:
    fail('[위조 의심] manifest 서명 불일치')
os.chdir('..')
log.info('서명 검증 통과 : CA 지문 → publisher.crt → manifest')

info, hashes = read_manifest('work/manifest.txt')
log.info(f'manifest : {info}')
if info.get('target') != DEVICE:
    fail(f"[호환성 불일치] {info.get('target')} 용 펌웨어 → 이 기기({DEVICE})에 설치 불가")
if datetime.datetime.fromisoformat(info['expires']) < datetime.datetime.now(datetime.timezone.utc):
    fail(f"[만료] manifest 유효기간 {info['expires']} 지남")
if os.path.exists(VALID):  # 지금 정상 설치된 펌웨어가 있으면 버전 비교 (롤백 방지)
    installed = int(read_manifest(DIR + 'manifest.txt')[0]['version'])
    log.debug(f"버전 비교 : 설치됨 {installed} / 새 manifest {info['version']}")
    if int(info['version']) < installed:
        fail(f"[롤백 거부] 설치된 버전 {installed} → 새 버전 {info['version']} 은 더 낮음")
else:
    log.debug('정상 설치된 펌웨어 없음 (처음 설치 또는 업데이트 도중) → 버전 비교 생략')
log.info('호환성 / 만료 / 버전 확인 통과')
size, piece, count = int(info['size']), int(info['piece']), int(info['count'])


def chunk(i):  # i번 조각의 시작 위치와 끝 위치(포함)
    start = i * piece
    return start, min(start + piece, size) - 1


# 2) 덮어쓰기 시작 전 : valid 끄기 → 새 manifest 를 펌웨어 옆에 보관 (이제부터 해시가 펌웨어를 따라다님)
if os.path.exists(VALID):
    os.remove(VALID)
    log.info('valid 플래그 끔 → 업데이트가 끝날 때까지 펌웨어 사용 불가')
for name in SIGNED:
    shutil.copy('work/' + name, DIR + name)
log.debug(f'새 manifest·서명·인증서를 {DIR} 에 보관')
if not os.path.exists(FIRMWARE):  # 처음 설치 (빈 기기)
    open(FIRMWARE, 'wb').close()
    log.info('기존 펌웨어 없음 → 새로 설치')
if os.path.getsize(FIRMWARE) != size:
    log.debug(f'펌웨어 크기 조정 : {os.path.getsize(FIRMWARE)} → {size} 바이트')
    os.truncate(FIRMWARE, size)  # 새 펌웨어 크기에 맞춤

# 3) 펌웨어의 조각을 새 manifest 와 비교 → 틀린 조각만 받기 (진행 기록 파일 없이 해시로 판단)
with open(FIRMWARE, 'rb') as f:
    todo = []
    for i in range(count):
        start, end = chunk(i)
        f.seek(start)
        if sha256(f.read(end - start + 1)) != hashes[i]:
            todo.append(i)
log.info(f'조각 비교 : {count - len(todo)}/{count} 이미 새 펌웨어와 같음 → {len(todo)}개 받기')
log.debug(f'받을 조각 : {todo}')

stats = {'retry': 0, 'bytes': 0}  # 요약용 (재시도 횟수, 받은 바이트)


def download(i):  # 받기 → 메모리에서 검증 → 제자리에 덮어쓰기 → 다시 읽어 확인
    start, end = chunk(i)
    for attempt in range(1, RETRY + 1):
        t0 = time.time()
        try:
            content_range, data = get('firmware.bin', start, end)
        except (OSError, http.client.HTTPException) as e:  # 네트워크 끊김 → 다시 시도 (많이 나오므로 파일에만 기록)
            log.debug(f'조각 {i:3d} 시도 {attempt}/{RETRY} : 네트워크 오류 {e!r}')
            stats['retry'] += 1
            continue
        if content_range != f'bytes {start}-{end}/{size}':
            log.warning(f'조각 {i:3d} 시도 {attempt}/{RETRY} : 범위 불일치 {content_range} (요청 {start}-{end}), 받은 {len(data)} 바이트')
            stats['retry'] += 1
            continue
        got = sha256(data)
        if got != hashes[i]:
            log.warning(f'조각 {i:3d} 시도 {attempt}/{RETRY} : 해시 불일치 받음 {got[:16]}… / 기대 {hashes[i][:16]}… → 변조 의심')
            stats['retry'] += 1
            continue
        with open(FIRMWARE, 'r+b') as f:  # r+b : 파일을 지우지 않고 그 위치만 덮어씀
            f.seek(start)
            f.write(data)
            f.flush()
            os.fsync(f.fileno())  # 디스크에 확실히 씀
            f.seek(start)
            written = sha256(f.read(len(data)))  # 쓴 내용을 다시 읽어 확인 (쓰기 오류 대비)
        if written != hashes[i]:
            log.warning(f'조각 {i:3d} 시도 {attempt}/{RETRY} : 쓰기 후 재확인 실패 (디스크 {written[:16]}…)')
            stats['retry'] += 1
            continue
        stats['bytes'] += len(data)
        log.debug(f'조각 {i:3d} 완료 : {start}-{end}, {len(data)} 바이트, 해시 {got[:16]}…, 시도 {attempt}회, {time.time() - t0:.2f}초')
        return True
    log.debug(f'조각 {i:3d} : {RETRY}번 모두 실패')  # 화면에는 아래 다운로드 요약으로 한 번에 표시
    return False


t_download = time.time()
with ThreadPoolExecutor(WORKERS, thread_name_prefix='다운로드') as pool:
    results = list(pool.map(download, todo))
failed = [i for i, ok in zip(todo, results) if not ok]
elapsed = time.time() - t_download
log.info(f"다운로드 요약 : 성공 {len(todo) - len(failed)}/{len(todo)}개, {stats['bytes']:,} 바이트, "
         f"{elapsed:.1f}초 ({stats['bytes'] / 1048576 / max(elapsed, 0.001):.1f} MB/s), 재시도 {stats['retry']}회")
if failed:  # valid 가 꺼진 상태로 종료 → 반쯤 쓴 펌웨어는 실행되지 않음, 다시 실행하면 남은 조각만 받음
    log.debug(f'실패 조각 : {failed}')
    fail(f'[중단] {len(failed)}개 조각 실패 → 다시 실행하면 이어서 받음 (펌웨어 사용 불가 상태)')

# 4) 디스크에서 다시 읽어 조각 전체 재검증 → valid 켜기 (설치 확정)
with open(FIRMWARE, 'rb') as f:
    for i in range(count):
        start, end = chunk(i)
        f.seek(start)
        if sha256(f.read(end - start + 1)) != hashes[i]:
            fail(f'[실패] 조각 {i} 재검증 실패 → 다시 실행 필요 (펌웨어 사용 불가 상태)')
log.info(f'전체 재검증 통과 : 조각 {count}개')
with open(VALID, 'w') as f:
    f.write(info['version'])  # 플래그 파일에 설치된 버전 기록
    f.flush()
    os.fsync(f.fileno())
log.info(f"[성공] 설치 완료 : version {info['version']} ({DEVICE}) → {FIRMWARE}, 총 {time.time() - started:.1f}초")
log.info('===== 업데이트 종료 (성공) =====')
