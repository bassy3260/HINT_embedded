# [기기] OTA 업데이트 : python device/main.py <target>
import os, sys, json, time
from pathlib import Path
from download import download
from install import install

target = int(sys.argv[1])
MY_HW = f'board-{target}'  # 이 기기의 하드웨어 (실제 기기라면 출하 때 저장된 값, 서버에서 받지 않음)
work = Path(f'device/work/{target}')
boot_file = work / 'boot.json'  # 부팅할 칸 + 그 칸의 버전 (기기 저장소, 설치 끝에 한 번에 바뀜)
boot = json.loads(boot_file.read_text()) if boot_file.exists() else {'slot': None, 'version': 0}
my_ver = boot['version']  # 설치된 버전 (처음이면 0)
if boot.get('trial'):  # 새 펌웨어 확인 전에 또 설치하면 돌아갈 칸(이전 펌웨어)을 덮어씀
    raise SystemExit('[대기] 새 펌웨어가 아직 확인 전 → 먼저 python device/boot.py 로 부팅')
key_file = work / 'key_version.txt'  # 본 적 있는 가장 높은 제작자 키 버전 (기기 저장소)
my_key_ver = int(key_file.read_text()) if key_file.exists() else 0
bad_file = work / 'bad_version.txt'  # 부팅에 실패했던 버전 (boot.py 가 롤백할 때 기록)
bad_ver = int(bad_file.read_text()) if bad_file.exists() else 0
time_file = work / 'last_time.txt'  # 본 적 있는 가장 최근 manifest 서명 시각 (기기 저장소)
last_time = int(time_file.read_text()) if time_file.exists() else 0
clock = int(time.time()) + int(os.environ.get('CLOCK_SHIFT', 0))  # 기기 시계 (실험용 : CLOCK_SHIFT 초만큼 틀리게)
now = max(clock, last_time)  # 시계가 과거로 가도 본 적 있는 시각보다 뒤로는 안 감

manifest, key_ver = download(target, MY_HW, my_ver, bad_ver, my_key_ver, now, work)
install(work / 'chunks', manifest, work, boot)
key_file.write_text(str(key_ver))
time_file.write_text(str(max(last_time, manifest['issued_at'])))
print('설치 완료')
