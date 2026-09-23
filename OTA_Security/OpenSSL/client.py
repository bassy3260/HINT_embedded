import os, json, hashlib, subprocess, urllib.request  # 필요한 기본 모듈들 불러오기

OPENSSL = r'C:\Program Files\Git\mingw64\bin\openssl.exe'  # openssl 프로그램 위치 (Git 설치 시 같이 깔림)
def ssl(*args, data=None):  # openssl 명령을 쉽게 부르기 위한 함수 (args = 명령 옵션들, data = 넣어줄 데이터)
    return subprocess.run([OPENSSL, *args], input=data, capture_output=True, check=True).stdout  # openssl 실행 → 결과(바이트) 돌려줌, 실패하면 에러

URL = 'http://192.168.0.19:9999/'  # 서버 주소
get = lambda name: urllib.request.urlopen(URL + name).read()  # 서버에서 파일 하나 받아오는 함수 (예: get('A.enc'))

# 1) 받기
open('public.pem', 'wb').write(get('public.pem'))  # 서버 공개키 받아서 파일로 저장 (openssl 이 파일로 읽어야 해서)
key_enc = get('key.enc')  # 개인키로 암호화된 AES 키 받기
enc = get('firmware.enc')  # 암호화된 파일 A 받기
manifest = get('firmware.sha256').decode().strip()   # 14줄: 글자로 바꾸기
 # 해시가 적힌 manifest 받아서 딕셔너리로 변환

# 2) 해시 검증
if hashlib.sha256(enc).hexdigest() != manifest:  # 받은 암호문 지문 != 서버가 알려준 지문 이면
    raise SystemExit('해시 불일치 → 폐기')  # 누가 건드렸거나 깨진 것 → 프로그램 종료

# 3) 공개키로 AES 키 복원
key = ssl('pkeyutl', '-verifyrecover', '-pubin', '-inkey', 'public.pem', '-pkeyopt', 'rsa_padding_mode:none', data=key_enc)[-32:]  # 패딩 없이 풀고 뒤 32바이트 = AES 키

# 4) 복호화
os.makedirs('work/device', exist_ok=True)  # 저장할 폴더 만들기 (이미 있으면 그냥 넘어감)
open('work/device/A.bin', 'wb').write(ssl('enc', '-d', '-aes-256-cbc', '-K', key.hex(), '-iv', bytes(16).hex(), data=enc))  # AES 키 + IV(0)로 암호문 복호화(-d) → 원본 A.bin 저장
print('성공')  # 끝!
