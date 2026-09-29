# 부팅 검사 흉내 (Secure Boot) : 설치된 펌웨어를 실행하기 전에 매번 검증
#  1) valid 플래그가 있는지 (업데이트 도중이면 없음)
#  2) 펌웨어 옆에 보관된 manifest 의 서명이 맞는지 (CA 지문 → 인증서 → 서명)
#  3) 펌웨어의 모든 조각이 manifest 해시와 같은지
#  하나라도 틀리면 실행 거부. 결과는 client.py 와 같은 로그 파일(work/ota.log)에도 남김

import hashlib, logging, os, subprocess

OPENSSL = r'C:\Program Files\Git\mingw64\bin\openssl.exe'
CA_FINGERPRINT = '99:61:00:14:74:45:D9:5C:C1:CC:E8:29:21:2D:14:0D:FC:E8:B5:67:9E:9D:F3:41:62:2F:6E:E6:06:02:82:81'
DIR = 'work/device/'

log = logging.getLogger('boot')
log.setLevel(logging.INFO)
file_handler = logging.FileHandler('work/ota.log', encoding='utf-8')  # client.py 와 같은 로그 파일에 이어서 기록
file_handler.setFormatter(logging.Formatter('%(asctime)s [%(levelname)-5s] [부팅검사    ] %(message)s'))
screen_handler = logging.StreamHandler()
screen_handler.setFormatter(logging.Formatter('%(asctime)s [%(levelname)s] %(message)s', '%H:%M:%S'))
log.addHandler(file_handler)
log.addHandler(screen_handler)


def refuse(message):
    log.error('[부팅 거부] ' + message)
    raise SystemExit(1)


def openssl(*args):
    return subprocess.run([OPENSSL, *args], capture_output=True, text=True, cwd=DIR)


# 1) 사용 가능 플래그
if not os.path.exists(DIR + 'valid'):
    refuse('valid 플래그 없음 → 업데이트가 끝나지 않은 펌웨어')

# 2) 보관된 manifest 서명 검증
if openssl('x509', '-in', 'ca.crt', '-noout', '-fingerprint', '-sha256').stdout.split('=')[-1].strip() != CA_FINGERPRINT:
    refuse('CA 지문 불일치')
if openssl('verify', '-CAfile', 'ca.crt', 'publisher.crt').returncode != 0:
    refuse('publisher.crt 검증 실패')
openssl('x509', '-in', 'publisher.crt', '-noout', '-pubkey', '-out', 'publisher_pub.pem')
if openssl('pkeyutl', '-verify', '-pubin', '-inkey', 'publisher_pub.pem', '-rawin',
           '-in', 'manifest.txt', '-sigfile', 'manifest.txt.sig').returncode != 0:
    refuse('manifest 서명 불일치')

# 3) 조각 전부 검증
info, hashes = {}, {}
for line in open(DIR + 'manifest.txt'):
    key, *values = line.split()
    if key.isdigit():
        hashes[int(key)] = values[0]
    else:
        info[key] = ' '.join(values)
piece = int(info['piece'])
with open(DIR + 'firmware.bin', 'rb') as f:
    for i in range(int(info['count'])):
        if hashlib.sha256(f.read(piece)).hexdigest() != hashes[i]:  # 순서대로 piece 씩 읽음 (마지막은 남은 만큼)
            refuse(f'조각 {i} 가 manifest 와 다름 → 펌웨어 손상/변조')
log.info(f"[부팅 OK] version {info['version']} / {info['target']} / 조각 {info['count']}개 검증 통과")
