# [사전 준비] Root CA 만들기 (맨 처음 한 번)
#  - CA 비밀키 : prepare/ca/ca.key    → 금고에 보관 (제작자 인증서 발급할 때만 사용)
#  - CA 인증서 : device/trust/ca.crt  → 출하 때 기기에 넣어두는 것 (기기는 이것만 믿음)
import shutil, subprocess
from pathlib import Path

KEY = Path('prepare/ca/ca.key')
CRT = Path('prepare/ca/ca.crt')

if KEY.exists():  # CA 를 다시 만들면 출하된 기기가 전부 못 믿게 됨
    raise SystemExit(f'이미 있음 : {KEY} (새로 만들려면 직접 지우고 실행)')

KEY.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(['openssl', 'genpkey', '-algorithm', 'ed25519', '-out', KEY], check=True)
subprocess.run(['openssl', 'req', '-x509', '-new', '-key', KEY, '-subj', '/CN=OTA Root CA', '-days', '3650',
                '-addext', 'basicConstraints=critical,CA:TRUE',
                '-addext', 'keyUsage=critical,keyCertSign',
                '-out', CRT], check=True)

Path('device/trust').mkdir(parents=True, exist_ok=True)
shutil.copy(CRT, 'device/trust/ca.crt')
print('CA 비밀키 :', KEY)
print('CA 인증서 :', CRT, '→ device/trust/ca.crt')
