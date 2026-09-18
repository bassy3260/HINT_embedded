import os, sys, cv2
from gpiozero import LED, Button
from signal import pause

if len(sys.argv) < 2:
    raise SystemExit("사용법: python collect_dataset.py <클래스명>")

label = sys.argv[1]
OUT = os.path.expanduser(f'~/work/dataset/{label}')
os.makedirs(OUT, exist_ok=True)

led = LED(17)
button = Button(27, bounce_time=0.1)

cap = cv2.VideoCapture(0, cv2.CAP_V4L2)
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

count = len([f for f in os.listdir(OUT) if f.endswith('.jpg')])