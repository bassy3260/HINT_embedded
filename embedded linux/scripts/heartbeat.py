# ~/work/scripts/heartbeat.py
from gpiozero import LED
from time import sleep
import socket

led = LED(17)
print("heartbeat start on", socket.gethostname(), flush=True)   # journal에 기록됨

n = 0
while True:
    led.on();  sleep(0.1)
    led.off(); sleep(0.9)
    n += 1
    if n % 60 == 0:
        print("alive", n, flush=True)