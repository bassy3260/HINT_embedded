# [사전 준비] 서버 폴더의 manifest.json 마다 제작자 비밀키로 서명 → manifest.json.sig
#             + 제작자 인증서를 서버에 올림 (기기가 CA 로 확인할 수 있게)
import shutil, subprocess
from pathlib import Path

KEY = 'prepare/keys/publisher.key'

for m in sorted(Path('server/files').glob('*/manifest.json')):
    sig = m.with_name('manifest.json.sig')
    subprocess.run(['openssl', 'pkeyutl', '-sign', '-inkey', KEY, '-rawin', '-in', m, '-out', sig], check=True)
    print('서명 :', sig)

shutil.copy('prepare/keys/publisher.crt', 'server/files/publisher.crt')
print('인증서 : server/files/publisher.crt')
