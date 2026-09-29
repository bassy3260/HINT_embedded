# [사전 준비] 타겟 1~10 용 랜덤 펌웨어를 만들어 암호화 → 서버 폴더에 firmware.enc + manifest.json
#             (청크로 자르지 않음. 청크 해시는 암호문을 읽으며 계산만 해서 manifest 에 넣음)
#  - 사용법 : python prepare/make_firmware.py <버전> [유효기간(분)]   (예: 2, 생략하면 1 / 7일)
#  - 평문 원본은 서버가 아니라 prepare/plain/<타겟>.bin 에 (결과 비교용)
import os, sys, json, time, secrets, hashlib, subprocess
from pathlib import Path

SIZE = 1024 * 1024  # 펌웨어 1개 = 1 MiB
CHUNK = 64 * 1024   # 청크 해시 단위 = 64 KiB
VERSION = int(sys.argv[1]) if len(sys.argv) > 1 else 1  # 새로 배포할 때마다 올린다
VALID_MIN = int(sys.argv[2]) if len(sys.argv) > 2 else 7 * 24 * 60  # manifest 유효기간(분), 음수면 이미 만료
NOW = int(time.time())
FW_KEY = Path('prepare/fw_key.hex').read_text().strip()  # 펌웨어 암호화 키 (make_fw_key.py)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


Path('prepare/plain').mkdir(parents=True, exist_ok=True)
for t in range(1, 11):
    fw = os.urandom(SIZE)  # 타겟마다 다른 랜덤 펌웨어 (평문)
    iv = secrets.token_hex(16)  # 펌웨어마다 새 IV (같은 키로 같은 IV 를 다시 쓰면 안 됨)
    enc = subprocess.run(['openssl', 'enc', '-aes-256-ctr', '-K', FW_KEY, '-iv', iv],
                         input=fw, capture_output=True, check=True).stdout
    out = Path(f'server/files/{t}')
    out.mkdir(parents=True, exist_ok=True)

    manifest = {
        'hw': f'board-{t}',  # 이 펌웨어가 돌아갈 하드웨어
        'version': VERSION,  # 펌웨어 버전 (클수록 최신)
        'issued_at': NOW,                  # 서명한 시각 (유닉스 시간, 초)
        'expires': NOW + VALID_MIN * 60,   # 이 시각이 지나면 기기가 거부
        'size': SIZE,
        'chunk_size': CHUNK,
        'chunk_sha256': [sha256(enc[i:i + CHUNK]) for i in range(0, SIZE, CHUNK)],  # 암호문 청크마다 해시
        'sha256': sha256(enc),       # 암호문 전체 해시 (다운로드 확인용)
        'iv': iv,                    # 복호화에 필요 (비밀 아님)
        'plain_sha256': sha256(fw),  # 복호화한 평문 해시 (설치·부팅 확인용)
    }
    (out / 'firmware.enc').write_bytes(enc)
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2))
    Path(f'prepare/plain/{t}.bin').write_bytes(fw)
    print(f'target {t} : v{VERSION}, {SIZE} 바이트 (암호화), 청크 해시 {len(manifest["chunk_sha256"])}개')
print('만료 :', time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(NOW + VALID_MIN * 60)))
