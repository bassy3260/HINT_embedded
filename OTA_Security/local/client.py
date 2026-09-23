import os
import urllib.request

URL="http://192.168.0.37:8000/Test.py"
blob = urllib.request.urlopen(URL).read()

DEST = './work/device/Test.py'
os.makedirs('./work/device', exist_ok=True)
open(DEST, 'wb').write(blob)


print('다운로드 :',len(blob),'바이트')
print('기록 :',DEST);