import csv, os, shutil, time
from datetime import datetime

LOG = os.path.expanduser('~/work/logs/sysinfo.csv')
MAX_BYTES =1024       # 10MB 임시로 작게 변경
HEADER = ['time', 'temp_c', 'disk_free_gb', 'mem_avail_gb']


def cpu_temp():
    with open('/sys/class/thermal/thermal_zone0/temp') as f:
        return int(f.read()) / 1000


def mem_available_gb():
    with open('/proc/meminfo') as f:
        for line in f:
            if line.startswith('MemAvailable'):
                return int(line.split()[1]) / 1024 / 1024


def open_log():
    """로그 파일을 열고 (파일객체, writer)를 돌려준다. 새 파일이면 헤더도 쓴다."""
    new_file = not os.path.exists(LOG)
    f = open(LOG, 'a', newline='')
    w = csv.writer(f)
    if new_file:
        w.writerow(HEADER)
        f.flush()
    return f, w


def rotate_if_needed(f, w):
    """크기가 넘으면 닫고 이름 바꾸고 새로 연다."""
    if os.path.getsize(LOG) < MAX_BYTES:
        return f, w                      # 아직 작으면 그대로

    f.close()                            # ① 먼저 닫고
    stamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    os.rename(LOG, f"{LOG}.{stamp}")     # ② 이름 변경
    print(f"rotated -> {LOG}.{stamp}")
    return open_log()                    # ③ 새 파일로 다시 열기


f, w = open_log()
try:
    while True:
        w.writerow([datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                    f"{cpu_temp():.1f}",
                    f"{shutil.disk_usage('/').free/1024**3:.1f}",
                    f"{mem_available_gb():.2f}"])
        f.flush()
        f, w = rotate_if_needed(f, w)
        time.sleep(1)
except KeyboardInterrupt:
    f.close()
    print("stopped")