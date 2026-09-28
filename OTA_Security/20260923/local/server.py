import  http.server
import hashlib
import os
import json

print('배포서버 http://192.168.0.26/A.bin');

FILE = 'A.bin'

h = hashlib.sha256()
with open(FILE, 'rb') as f:
    for chunk in iter(lambda: f.read(4096), b''):
        h.update(chunk)
digest = h.hexdigest()

manifest = {
    'file': FILE,
    'size':os.path.getsize(FILE),
    'sha256': digest
}

with open('manifest.json', 'w') as f:
    json.dump(manifest, f, indent=2)

print('SHA256 :',digest)
print('배포서버 : http://<ip>:8000/'+FILE)
print('매니페스트 http://<ip>:8000/manifest.json');

h= hash
http.server.HTTPServer(('',8000),
                       http.server.SimpleHTTPRequestHandler).serve_forever()
