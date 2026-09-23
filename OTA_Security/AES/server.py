import http.server, hashlib, os, json, functools
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from Crypto.Random import get_random_bytes

KEY = bytes.fromhex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")  # 32바이트 = AES-256
 # 기기와 공유하는 키
SRC = 'A.bin'              # 원본 (서비스하지 않음)
OUT_DIR = 'dist'           # 이 폴더만 외부에 공개
ENC = 'A.enc'

os.makedirs(OUT_DIR, exist_ok=True)

plain = open(SRC, 'rb').read()
iv = get_random_bytes(16)                      # 매번 랜덤 IV
ct = AES.new(KEY, AES.MODE_CBC, iv).encrypt(pad(plain, AES.block_size))
blob = iv + ct                                 # 앞 16바이트 = IV

open(os.path.join(OUT_DIR, ENC), 'wb').write(blob)

manifest = {
    'file': ENC, # 암호문 파일명
    'size': len(blob), # 암호문 크기
    'sha256': hashlib.sha256(blob).hexdigest(),         # 암호문 해시
    'plain_sha256': hashlib.sha256(plain).hexdigest(),  # 평문 해시
}

# manifest.json 생성
json.dump(manifest, open(os.path.join(OUT_DIR, 'manifest.json'), 'w'), indent=2)
print(manifest)

Handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=OUT_DIR)
http.server.HTTPServer(('', 8000), Handler).serve_forever()
