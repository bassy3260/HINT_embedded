import  http.server

print('배포서버 http://<ip>/F.bin');

http.server.HTTPServer(('',8000),
                       http.server.SimpleHTTPRequestHandler).serve_forever()
