# [서버] server/files 의 파일을 그대로 제공 (Range 요청이면 그 구간만)
#  - 사용법 : python server/server.py [방해모드]
#  - 방해모드 : firmware.enc 요청 중 30% 확률로 방해 (manifest 는 정상 제공)
#      error   : 500 에러 응답
#      drop    : 절반만 보내고 연결 끊기
#      slow    : 15초 멈췄다 보내기 (기기 타임아웃 10초보다 김)
#      corrupt : 바이트 하나 뒤집어서 보내기
#      wrong   : 요청과 다른 구간 보내기 (1바이트 밀림)
#      full    : Range 무시하고 파일 전체 보내기
#      drip    : 1초에 1바이트씩 질질 흘려보내기 (기기 타임아웃 10초에 안 걸림)
#      mix     : 위 방해 중 하나를 매번 무작위로 (drip 제외)
import re, sys, time, random
from pathlib import Path
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler

ROOT = Path('server/files')
MODES = ['error', 'drop', 'slow', 'corrupt', 'wrong', 'full']
MODE = sys.argv[1] if len(sys.argv) > 1 else 'normal'
RATE = 0.3  # 방해 확률


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        f = ROOT / self.path.lstrip('/')
        if '..' in self.path or not f.is_file():
            return self.send_error(404)
        data = f.read_bytes()
        size = len(data)

        # 이번 요청을 방해할지 결정
        bad = None
        if MODE != 'normal' and f.name == 'firmware.enc' and random.random() < RATE:
            bad = random.choice(MODES) if MODE == 'mix' else MODE
            print(f'[방해 : {bad}] {self.path} {self.headers.get("Range")}')

        if bad == 'error':
            return self.send_error(500)

        # Range: bytes=시작-끝  → 그 구간만 206 으로
        m = re.fullmatch(r'bytes=(\d+)-(\d*)', self.headers.get('Range', ''))
        if m and bad != 'full':
            start = int(m[1])
            end = int(m[2]) if m[2] else size - 1
            if bad == 'wrong':
                start, end = start + 1, end + 1
            data = data[start:end + 1]
            self.send_response(206)
            self.send_header('Content-Range', f'bytes {start}-{start + len(data) - 1}/{size}')
        else:
            self.send_response(200)
        if bad == 'corrupt':
            data = bytearray(data)
            data[random.randrange(len(data))] ^= 0xFF
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()

        if bad == 'drop':
            self.wfile.write(data[:len(data) // 2])
            return  # 나머지는 안 보내고 끝 → 연결 끊김
        if bad == 'slow':
            time.sleep(15)
        if bad == 'drip':
            try:
                for b in data:
                    self.wfile.write(bytes([b]))
                    time.sleep(1)
            except OSError:  # 기기가 연결을 끊으면 그만
                pass
            return
        self.wfile.write(data)


print(f'서버 시작 : http://192.168.0.29:9999/ (방해 모드 : {MODE})')
ThreadingHTTPServer(('192.168.0.29', 9999), Handler).serve_forever()
