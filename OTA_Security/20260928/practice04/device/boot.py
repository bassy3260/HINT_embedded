# [기기] 부트로더 흉내 : python device/boot.py <target>
#  - 보안 부팅 : 칸 옆에 보관한 manifest 를 CA → 인증서 → 서명 순서로 다시 확인하고, 펌웨어 해시·하드웨어 확인
#    (boot.json 같은 기기 파일에 적힌 값은 믿지 않음)
#  - 새 펌웨어는 시험 부팅 : 켜진 뒤 스스로 확인 도장을 찍어야 확정
#  - 확인 도장 없이 다시 켜지면 (= 지난번에 죽었다) 이전 칸으로 롤백 + 실패한 버전 기록
import os, sys, json
from pathlib import Path
from verify import verify, verify_cert, verify_sig, publisher_key_version
from install import save_boot

target = sys.argv[1]
work = Path(f'device/work/{target}')
boot = json.loads((work / 'boot.json').read_text())

if boot.get('tried'):  # 시험 부팅을 했는데 확인 도장이 없다 → 새 펌웨어가 죽었다
    prev = boot['prev']
    print(f"[롤백] slot {boot['slot']} (v{boot['version']}) 확인 안 됨 → slot {prev['slot']} (v{prev['version']}) 로")
    (work / 'bad_version.txt').write_text(str(boot['version']))  # 이 버전은 다시 받지 않게
    boot = prev
    save_boot(work, boot)
elif boot.get('trial'):
    boot['tried'] = True  # 켜기 전에 "시험했음" 기록 → 여기서 죽으면 다음 부팅 때 롤백
    save_boot(work, boot)

# 보안 부팅 : 서명부터 다시 확인 (부팅 때는 시계를 믿기 어려워 인증서 유효기간은 보지 않음)
s = work / f"slot_{boot['slot']}"
m, sig, cert = Path(f'{s}.manifest.json'), Path(f'{s}.manifest.json.sig'), Path(f'{s}.publisher.crt')
if not (verify_cert(cert) and publisher_key_version(cert) is not None and verify_sig(m, sig, cert)):
    raise SystemExit(f"[부팅 거부] slot {boot['slot']} : 보관된 manifest 의 서명 확인 실패")
manifest = json.loads(m.read_bytes())
fw = Path(f'{s}.bin').read_bytes()
if manifest['hw'] != f'board-{target}' or not verify(fw, manifest['plain_sha256']):
    raise SystemExit(f"[부팅 거부] slot {boot['slot']} : 펌웨어가 서명된 manifest 와 다름")
print(f"부팅 : slot {boot['slot']}, v{manifest['version']} (서명·해시 확인)")

# ---- 여기부터 펌웨어가 실행 중이라고 생각 ----
if str(manifest['version']) == os.environ.get('CRASH_VERSION'):  # 실험용 : 이 버전은 켜지자마자 죽음
    print('[크래시] 펌웨어가 죽음 (확인 도장 못 찍음) → 다시 부팅하면?')
    os._exit(1)
if boot.get('trial'):  # 잘 켜졌다 → 확인 도장 : 이 칸으로 확정
    save_boot(work, {'slot': boot['slot'], 'version': boot['version']})
    print('확인 도장 : 이 칸으로 확정')
