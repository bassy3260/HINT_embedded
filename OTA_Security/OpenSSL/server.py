import os, json, hashlib, subprocess, functools, http.server  # 필요한 기본 모듈들 불러오기

OPENSSL = r'C:\Program Files\Git\mingw64\bin\openssl.exe'  # openssl 프로그램 위치 (Git 설치 시 같이 깔림)
def ssl(*args, data=None):  # openssl 명령을 쉽게 부르기 위한 함수 (args = 명령 옵션들, data = 넣어줄 데이터)
    return subprocess.run([OPENSSL, *args], input=data, capture_output=True, check=True).stdout  # openssl 실행 → 결과(바이트) 돌려줌, 실패하면 에러

os.makedirs('dist', exist_ok=True)  # 외부에 공개할 폴더 'dist' 만들기 (이미 있으면 그냥 넘어감)

# 1) RSA 키 만들기
ssl('genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', 'priv.pem')  # RSA-2048 개인키 생성 → priv.pem (서버만 가짐, dist 밖)
ssl('pkey', '-in', 'priv.pem', '-pubout', '-out', 'dist/pub.pem')  # 개인키에서 짝꿍 공개키 뽑기 → dist/pub.pem (배포용)

# 2) 파일 A 암호화 (AES-256-CBC, IV=0)
key = os.urandom(32)  # 랜덤 32바이트 = AES-256 대칭키
enc = ssl('enc', '-aes-256-cbc', '-K', key.hex(), '-iv', bytes(16).hex(), '-in', 'A.bin')  # A.bin 을 AES 키 + IV(0 x 16바이트)로 암호화 → 암호문 바이트
open('dist/A.enc', 'wb').write(enc)  # 암호문을 dist/A.enc 로 저장

# 3) AES 키를 개인키로 암호화
open('dist/key.enc', 'wb').write(ssl('pkeyutl', '-sign', '-inkey', 'priv.pem', data=key))  # AES 키를 개인키로 암호화 → dist/key.enc (공개키로만 열림)

# 4) 해시
json.dump({'sha256': hashlib.sha256(enc).hexdigest()}, open('dist/manifest.json', 'w'))  # 암호문의 SHA-256 지문을 manifest.json 에 저장

http.server.HTTPServer(('', 8000), functools.partial(http.server.SimpleHTTPRequestHandler, directory='dist')).serve_forever()  # 8000번 포트로 dist 폴더만 웹서버로 공개 (계속 실행)
