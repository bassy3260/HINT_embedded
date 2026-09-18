import os, cv2
from gpiozero import LED

led = LED(17)

# cascade 파일 위치: pip 설치는 cv2.data,
# apt(opencv-data) 설치는 /usr/share/opencv4
cands = []
if hasattr(cv2, 'data'):
    cands.append(cv2.data.haarcascades
                 + 'haarcascade_frontalface_default.xml')
cands.append('/usr/share/opencv4/haarcascades/'
             'haarcascade_frontalface_default.xml')

path = next((p for p in cands if os.path.exists(p)), None)
if path is None:
    raise SystemExit("cascade 없음: sudo apt install opencv-data")

detector = cv2.CascadeClassifier(path)

cap = cv2.VideoCapture(0, cv2.CAP_V4L2)
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

print("얼굴이 보이면 LED 점등. Ctrl+C 종료")
try:
    while True:
        ok, frame = cap.read()
        if not ok:
            continue
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        faces = detector.detectMultiScale(
            gray, scaleFactor=1.2, minNeighbors=5,
            minSize=(60, 60))
        if len(faces) > 0:
            led.on()
        else:
            led.off()
        print(f"faces={len(faces)}  ", flush=True)
except KeyboardInterrupt:
    pass
finally:
    led.off()
    cap.release()
    print()