import hashlib
import secrets

# 임의의 메시지 생성 (16바이트 랜덤 → hex 문자열)
msg = "hello"
# SHA-256 해시 계산
digest = hashlib.sha256(msg.encode()).hexdigest()

print("Message:", msg)
print("SHA-256:", digest)

# 임의의 메시지 생성 (16바이트 랜덤 → hex 문자열)
msg = "H2313233h3213ello"
# SHA-256 해시 계산
digest = hashlib.sha256(msg.encode()).hexdigest()

print("newMessage:", msg)
print("SHA-256:", digest)