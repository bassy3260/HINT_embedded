import hashlib, subprocess, urllib.request

OPENSSL = r'C:\Program Files\Git\mingw64\bin\openssl.exe'
def ssl(*args, data=None):  # openssl 실행 → 결과(바이트), 실패하면 에러
    return subprocess.run([OPENSSL, *args], input=data, capture_output=True, check=True).stdout

URL = 'http://192.168.0.26:8000/'
get = lambda name: urllib.request.urlopen(URL + name).read()  # 서버에서 파일 받기

# 1) 미리 넣어 둔 서버 인증서(WSY_End.crt)에서 공개키 꺼내기
open('pub.pem', 'wb').write(ssl('x509', '-pubkey', '-noout', '-in', '../key/WSY_End.crt'))

# 2) 조각을 순서대로 받기 → 서명 검증 → 해시 검증 → 이어 붙이기
with open('merged.bin', 'wb') as out:
    for n in range(int(get('count'))):
        part = get(f'chunks/{n}.bin')
        h = get(f'chunks/{n}.sha256')
        open('sig', 'wb').write(get(f'chunks/{n}.sha256.sig'))
        ssl('dgst', '-sha256', '-verify', 'pub.pem', '-signature', 'sig', data=h)  # 서명 검증 (틀리면 에러)
        if hashlib.sha256(part).hexdigest().encode() != h:  # 해시 검증
            raise SystemExit(f'{n}번 조각 해시 불일치')
        out.write(part)

# 3) 원본과 같은지 확인
print('원본과 동일' if open('merged.bin', 'rb').read() == open('big.bin', 'rb').read() else '원본과 다름')
