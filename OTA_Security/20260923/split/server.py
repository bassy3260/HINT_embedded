import os, hashlib, subprocess, functools, http.server

OPENSSL = r'C:\Program Files\Git\mingw64\bin\openssl.exe'
def ssl(*args, data=None):  # openssl 실행 → 결과(바이트), 실패하면 에러
    return subprocess.run([OPENSSL, *args], input=data, capture_output=True, check=True).stdout

CHUNK = 64 * 1024  # 조각 크기 64KB
os.makedirs('dist/chunks', exist_ok=True)  # dist 폴더만 외부에 공개
KEY = '../key/WSY_End.pem'  # 서명에 쓸 개인키 (XCA 에서 내보낸 End Entity 키)

# 1) 100MB 랜덤 파일 만들기
if not os.path.exists('big.bin'):
    open('big.bin', 'wb').write(os.urandom(100 * 1024 * 1024))
data = open('big.bin', 'rb').read()

# 2) 64KB 씩 쪼개서 조각, 해시, 서명 만들기
count = 0
for i in range(0, len(data), CHUNK):
    part = data[i:i + CHUNK]  # 조각
    h = hashlib.sha256(part).hexdigest().encode()  # 조각의 해시
    open(f'dist/chunks/{count}.bin', 'wb').write(part)
    open(f'dist/chunks/{count}.sha256', 'wb').write(h)
    open(f'dist/chunks/{count}.sha256.sig', 'wb').write(ssl('dgst', '-sha256', '-sign', KEY, data=h))  # 해시를 개인키로 서명
    count += 1
open('dist/count', 'w').write(str(count))  # 조각 개수
print('준비 완료, 조각', count, '개')

http.server.HTTPServer(('', 8000), functools.partial(http.server.SimpleHTTPRequestHandler, directory='dist')).serve_forever()
