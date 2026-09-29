# [사전 준비] 제작자(publisher) 서명 키 만들기 + CA 에게 인증서 받기
#  - 비밀키 : prepare/keys/publisher.key  → 개발자 PC 에만 (서버·기기에 절대 X)
#  - 인증서 : prepare/keys/publisher.crt  → "이 공개키는 진짜 제작자 것, 용도는 펌웨어 서명만" (CA 도장)
#  - 사용법 : python prepare/make_keys.py <키 버전>   (예: 2, 생략하면 1)
#  - 키를 교체할 때 : prepare/keys 를 지우고 버전을 올려 다시 실행 (기기는 그대로 둬도 됨)
import sys, secrets, subprocess
from pathlib import Path

KEY = Path('prepare/keys/publisher.key')
CRT = Path('prepare/keys/publisher.crt')
CA_KEY, CA_CRT = 'prepare/ca/ca.key', 'prepare/ca/ca.crt'
KEY_VER = int(sys.argv[1]) if len(sys.argv) > 1 else 1  # 키를 교체할 때마다 올린다

if KEY.exists():
    raise SystemExit(f'이미 있음 : {KEY} (교체하려면 prepare/keys 를 지우고 버전을 올려 실행)')

KEY.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(['openssl', 'genpkey', '-algorithm', 'ed25519', '-out', KEY], check=True)

# CA 가 도장 찍어서 인증서 발급 : 이름에 키 버전, 용도는 펌웨어 서명(codeSigning) 전용
subprocess.run(['openssl', 'req', '-x509', '-new', '-key', KEY, '-subj', f'/CN=OTA Publisher/OU=key-{KEY_VER}',
                '-CA', CA_CRT, '-CAkey', CA_KEY, '-days', '365', '-set_serial', str(secrets.randbits(63)),
                '-addext', 'basicConstraints=critical,CA:FALSE',
                '-addext', 'keyUsage=critical,digitalSignature',
                '-addext', 'extendedKeyUsage=critical,codeSigning',
                '-out', CRT], check=True, capture_output=True)
print('비밀키 :', KEY)
print('인증서 :', CRT, f'(CA 발급, key-{KEY_VER}, 펌웨어 서명 전용)')
