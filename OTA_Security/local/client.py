import os
import urllib.request
import hmac
import hashlib
import json 

URL="http://192.168.0.26:8000"
blob = urllib.request.urlopen(URL).read()
DEST = './work/device/A.bin'
TMP = DEST+".tmp"

os.makedirs('./work/device', exist_ok=True)

manifest = json.loads(urllib.request.urlopen(URL + '/manifest.json').read())
expected_sha256 = manifest['sha256']
expected_size = manifest['size']
print('기대 SHA256 :', expected_sha256)

h= hashlib.sha256()
size = 0
with urllib.request.urlopen(URL + '/' + manifest['file']) as r, open(TMP, 'wb') as f:
    for chunk in iter(lambda: r.read(4096), b''):
        f.write(chunk)
        h.update(chunk)
        size += len(chunk)
actual_hash = h.hexdigest()
print('계산 SHA-256 :', actual_hash)

# 3) 검증 (크기 → 해시)
if size != expected_size:
    os.remove(TMP)
    raise SystemExit(f'[실패] 크기 불일치 {size} != {expected_size}')

if not hmac.compare_digest(actual_hash, expected_sha256):
    os.remove(TMP)
    raise SystemExit('[실패] 해시 불일치 → 파일 폐기')

# 4) 통과 시에만 교체 (원자적)
os.replace(TMP, DEST)
print('[성공] 검증 완료 :', size, '바이트 →', DEST)


print('다운로드 :',len(blob),'바이트')
print('기록 :',DEST);