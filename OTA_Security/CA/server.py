import os, hashlib, subprocess, functools, http.server  # 필요한 기본 모듈들 불러오기

OPENSSL = r'C:\Program Files\Git\mingw64\bin\openssl.exe'  # openssl 프로그램 위치 (Git 설치 시 같이 깔림)
def ssl(*args, data=None):  # openssl 명령을 쉽게 부르기 위한 함수 (args = 명령 옵션들, data = 넣어줄 데이터)
    return subprocess.run([OPENSSL, *args], input=data, capture_output=True, check=True).stdout  # openssl 실행 → 결과(바이트) 돌려줌, 실패하면 에러

os.makedirs('dist', exist_ok=True)  # 외부에 공개할 폴더 'dist' 만들기 (이미 있으면 그냥 넘어감)

# 1) RSA 키 만들기
ssl('genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', 'priv.pem')  # RSA-2048 개인키 생성 → priv.pem (서버만 가짐, dist 밖)
ssl('pkey', '-in', 'priv.pem', '-pubout', '-out', 'dist/public.pem')  # 개인키에서 짝꿍 공개키 뽑기 → dist/public.pem (배포용)

# 2) 펌웨어 암호화 (AES-256-CBC, IV=0)
key = os.urandom(32)  # 랜덤 32바이트 = AES-256 대칭키
enc = ssl('enc', '-aes-256-cbc', '-K', key.hex(), '-iv', bytes(16).hex(), '-in', 'firmware.bin')  # firmware.bin 을 AES 키 + IV(0 x 16바이트)로 암호화 → 암호문 바이트
open('dist/firmware.enc', 'wb').write(enc)  # 암호문을 dist/firmware.enc 로 저장

# 3) AES 키를 개인키로 암호화 (패딩 없음)
padded = bytes(256 - 32) + key  # 앞을 0으로 채워 256바이트로 맞춤 (RSA-2048 한 블록 크기, 클라이언트는 뒤 32바이트를 꺼냄)
open('dist/key.enc', 'wb').write(ssl('rsautl', '-sign', '-raw', '-inkey', 'priv.pem', data=padded))  # 개인키로 raw RSA 암호화 → dist/key.enc (pkeyutl 은 raw 서명을 막아서 rsautl 사용)

# 4) 해시
open('dist/firmware.sha256', 'w').write(hashlib.sha256(enc).hexdigest())  # 암호문의 SHA-256 지문(64글자)을 dist/firmware.sha256 에 저장

# 5) 해시 서명
ssl('dgst', '-sha256', '-sign', 'priv.pem', '-out', 'dist/firmware.sha256.sig', 'dist/firmware.sha256')  # 해시 파일을 개인키로 서명 → dist/firmware.sha256.sig (공개키로 진짜인지 확인 가능)

http.server.HTTPServer(('', 9999), functools.partial(http.server.SimpleHTTPRequestHandler, directory='dist')).serve_forever()  # 9999번 포트로 dist 폴더만 웹서버로 공개 (계속 실행)
