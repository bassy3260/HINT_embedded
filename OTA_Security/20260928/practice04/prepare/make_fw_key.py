# [사전 준비] 펌웨어 암호화 키 만들기 (맨 처음 한 번)
#  - prepare/fw_key.hex       → 개발자 PC (펌웨어 암호화할 때)
#  - device/trust/fw_key.hex  → 출하 때 기기에 넣어두는 것 (복호화할 때). 서버에는 절대 X
#  - 모든 기기가 같은 키 (간단히 하기 위해) → 기기 한 대에서 털리면 모든 펌웨어가 풀림
import secrets, shutil
from pathlib import Path

KEY = Path('prepare/fw_key.hex')

if KEY.exists():
    raise SystemExit(f'이미 있음 : {KEY} (새로 만들려면 직접 지우고 실행)')

KEY.write_text(secrets.token_hex(32))  # AES-256 키 (32바이트)
Path('device/trust').mkdir(parents=True, exist_ok=True)
shutil.copy(KEY, 'device/trust/fw_key.hex')
print('암호화 키 :', KEY, '→ device/trust/fw_key.hex')
