from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad

key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")  # AES-128 키 (고정)
iv = bytes(16)                                             # IV = 0 (16바이트 0x00)
plaintext = b"hello"

# 1. 평문 암호화 (CBC는 16바이트 블록 단위 → PKCS#7 패딩)
padded = pad(plaintext, AES.block_size, style="pkcs7")
ct = AES.new(key, AES.MODE_CBC, iv).encrypt(padded)

s = key;
print("Key:       ", s.hex())
print("Key+IV:    ", s.hex())

# 2. 암호문 출력
print("Plaintext: ", plaintext)
print("Padded:    ", padded.hex())
print("Ciphertext:", ct.hex())

# 3. 복호화 (PKCS#7 패딩 제거)
pt = unpad(AES.new(key, AES.MODE_CBC, iv).decrypt(ct), AES.block_size, style="pkcs7")
print("Decrypted: ", pt)

# 4. 평문 일치 확인
print("Match:     ", pt == plaintext)
