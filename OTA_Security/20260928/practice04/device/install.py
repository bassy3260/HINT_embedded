# [기기] A/B 슬롯 설치 : 암호문 확인 → 복호화 → 평문 확인 → 지금 부팅하지 않는 칸에 쓰고, 부팅 칸 표시만 한 번에 바꾼다
#        새 펌웨어는 "시험 부팅" 상태로 → 부팅 후 확인 도장이 없으면 boot.py 가 이전 칸으로 롤백
import os, json, shutil, subprocess
from pathlib import Path
from verify import verify

FW_KEY = Path('device/trust/fw_key.hex')  # 출하 때 넣어둔 펌웨어 복호화 키 (서버에서 받지 않음)


def save_boot(work, boot):  # boot.json 을 한 번에 교체 (임시 파일에 쓰고 이름 바꾸기 → 중간 상태 없음)
    tmp = work / 'boot.json.tmp'
    tmp.write_text(json.dumps(boot))
    os.replace(tmp, work / 'boot.json')


def decrypt(enc, iv):
    return subprocess.run(['openssl', 'enc', '-d', '-aes-256-ctr', '-K', FW_KEY.read_text().strip(), '-iv', iv],
                          input=enc, capture_output=True, check=True).stdout


def install(chunk_dir, manifest, work, boot):
    n = len(manifest['chunk_sha256'])
    enc = b''.join((chunk_dir / f'{i}.bin').read_bytes() for i in range(n))
    if not verify(enc, manifest['sha256']):
        raise SystemExit('[실패] 암호문 전체 해시 불일치 → 설치 안 함')
    fw = decrypt(enc, manifest['iv'])
    if not verify(fw, manifest['plain_sha256']):  # 키가 틀리면 여기서 걸림
        raise SystemExit('[실패] 복호화한 펌웨어 해시 불일치 → 설치 안 함')
    print('복호화 OK')

    # 1) 지금 부팅하는 칸이 아닌 쪽에 쓴다 → 도중에 꺼져도 부팅 칸은 멀쩡
    slot = 'b' if boot['slot'] == 'a' else 'a'
    dest = work / f'slot_{slot}.bin'
    cut = int(os.environ.get('POWER_CUT', 0))  # 실험용 : 이 바이트만 쓰고 전원 차단 흉내
    with open(dest, 'r+b' if dest.exists() else 'wb') as f:  # 플래시처럼 제자리에 덮어쓰기
        if cut:
            f.write(fw[:cut])
            f.flush()
            print(f'[전원 차단] slot {slot} 에 {cut} 바이트 쓰고 꺼짐')
            os._exit(1)  # 뒷정리 없이 바로 종료
        f.write(fw)
        f.truncate()
    if not verify(dest.read_bytes(), manifest['plain_sha256']):  # 제대로 써졌나 다시 읽어서 확인
        raise SystemExit(f'[실패] slot {slot} 쓰기 확인 실패 → 부팅 칸 안 바꿈')
    for name in ('manifest.json', 'manifest.json.sig', 'publisher.crt'):  # 부팅 때 서명부터 다시 확인할 수 있게 보관
        shutil.copy(work / name, work / f'slot_{slot}.{name}')

    # 2) 커밋 : 부팅 칸 + 버전을 한 번에 바꾼다. 이전 칸이 있으면 "시험 부팅" + 돌아갈 곳(prev) 기록
    new = {'slot': slot, 'version': manifest['version']}
    if boot['slot']:
        new.update(trial=True, prev={'slot': boot['slot'], 'version': boot['version']})
    save_boot(work, new)
    print(f"부팅 칸 변경 : slot {slot} (v{manifest['version']})" + (' → 시험 부팅 대기' if boot['slot'] else ''))
