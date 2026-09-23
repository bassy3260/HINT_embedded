from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes

key = get_random_bytes(16)          # AES-128 키
msg = b"hello"

# 암호화 (GCM: 암호화 + 무결성 태그)
cipher = AES.new(key, AES.MODE_GCM)
ct, tag = cipher.encrypt_and_digest(msg)
nonce = cipher.nonce

print("Key:       ", key.hex())
print("Nonce:     ", nonce.hex())
print("Ciphertext:", ct.hex())
print("Tag:       ", tag.hex())

# 복호화 (태그가 안 맞으면 ValueError 발생)
cipher = AES.new(key, AES.MODE_GCM, nonce=nonce)
pt = cipher.decrypt_and_verify(ct, tag)
print("Decrypted: ", pt.decode())
