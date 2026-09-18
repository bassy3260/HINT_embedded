# ─── ① 초기화 ───────────────────────────
import argparse, csv, os, time
from datetime import datetime
import cv2
from gpiozero import LED, Button
from signal import pause

ap = argparse.ArgumentParser()
ap.add_argument('--interval', type=float, default=0)
ap.add_argument('--out', default=os.path.expanduser('~/work/data'))
args = ap.parse_args()

os.makedirs(args.out, exist_ok=True)
LOG = os.path.join(args.out, 'log.csv')

led = LED(17)
button = Button(27, bounce_time=0.1)

cap = cv2.VideoCapture(0, cv2.CAP_V4L2)
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

if not cap.isOpened():
    raise SystemExit("카메라 열기 실패")


# ─── ② 보조 함수 ────────────────────────
def cpu_temp():
    with open('/sys/class/thermal/thermal_zone0/temp') as f:
        return int(f.read()) / 1000


def write_log(row):
    new_file = not os.path.exists(LOG)
    with open(LOG, 'a', newline='') as f:
        w = csv.writer(f)
        if new_file:
            w.writerow(['time', 'file', 'width', 'height', 'cpu_temp'])
        w.writerow(row)


def error_blink():
    for _ in range(5):
        led.off(); time.sleep(0.1)
        led.on();  time.sleep(0.1)


# ─── ③ 촬영 함수 ────────────────────────
def capture():
    led.on()
    try:
        for _ in range(3):
            cap.read()
        ok, frame = cap.read()
        now = datetime.now()

        if not ok:
            print("capture failed"); error_blink(); return

        name = now.strftime('%Y%m%d_%H%M%S') + '.jpg'
        cv2.imwrite(os.path.join(args.out, name), frame)
        write_log([now.strftime('%Y-%m-%d %H:%M:%S'),
                   name, frame.shape[1], frame.shape[0],
                   f"{cpu_temp():.1f}"])
        print("saved", name)

    except Exception as e:
        print("error:", e); error_blink()
    finally:
        led.off()


# ─── ④ 메인 ─────────────────────────────
try:
    if args.interval > 0:
        print(f"{args.interval}초마다 촬영. Ctrl+C 종료")
        while True:
            capture()
            time.sleep(args.interval)
    else:
        button.when_pressed = capture
        print("버튼을 누르면 촬영. Ctrl+C 종료")
        pause()
except KeyboardInterrupt:
    pass
finally:
    cap.release()
    print("camera released")