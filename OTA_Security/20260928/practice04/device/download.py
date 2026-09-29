# [기기] 제작자 인증서(CA) 확인 → manifest 서명 검증 → Range 요청으로 암호문 청크 다운로드 + 구간·해시 검증 (실패하면 재시도)
import json, time, http.client, urllib.request
from verify import verify, verify_cert, verify_sig, publisher_key_version, check_manifest

SERVER = 'http://192.168.0.29:9999'
RETRY = 5           # 청크 하나당 최대 시도 횟수
TOTAL_TIMEOUT = 30  # 요청 하나를 다 받는 데 허용하는 전체 시간(초)
MAX_SIZE = 4 * 1024 * 1024  # 기기 슬롯 크기 (이보다 큰 펌웨어는 거부)


def get(path, headers={}, expect_range=None):
    req = urllib.request.Request(SERVER + path, headers=headers)
    deadline = time.monotonic() + TOTAL_TIMEOUT
    data = b''
    with urllib.request.urlopen(req, timeout=10) as r:  # 10초 동안 아무것도 안 오면 포기
        if expect_range and r.headers.get('Content-Range') != expect_range:  # 요청한 구간이 맞나 (본문 받기 전에 확인)
            raise http.client.HTTPException(f"요청과 다른 구간 : {r.headers.get('Content-Range')} (요청 {expect_range})")
        while piece := r.read1(65536):  # 온 만큼씩 읽으면서
            data += piece
            if time.monotonic() > deadline:  # 전체 시간이 넘으면 포기 (질질 끌기 방지)
                raise TimeoutError(f'{TOTAL_TIMEOUT}초 안에 다 못 받음')
    return data


def get_chunk(target, i, start, end, size, h):
    for n in range(RETRY):
        if n:
            time.sleep(2 ** (n - 1))  # 1, 2, 4, 8초 쉬고 다시 (서버에 몰리지 않게)
        try:
            data = get(f'/{target}/firmware.enc', {'Range': f'bytes={start}-{end}'}, f'bytes {start}-{end}/{size}')
            if verify(data, h):
                return data
            print(f'청크 {i} 해시 불일치 → 재시도')  # 검증 실패한 청크는 저장하지 않음
        except (OSError, http.client.HTTPException) as e:  # 끊김, 에러 응답, 타임아웃
            print(f'청크 {i} 받기 실패 ({type(e).__name__}: {e}) → 재시도')
    raise SystemExit(f'[실패] 청크 {i} : {RETRY}번 시도했지만 실패')


def fmt(t):
    return time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(t))


def download(target, my_hw, my_ver, bad_ver, my_key_ver, now, work):
    print('기기 시각 :', fmt(now))
    # 1) 제작자 인증서 받기 → CA 도장이 맞아야 그 안의 공개키를 믿는다
    work.mkdir(parents=True, exist_ok=True)
    c = work / 'publisher.crt'
    c.write_bytes(get('/publisher.crt'))
    if not verify_cert(c, now):
        raise SystemExit('[위조 의심] 제작자 인증서가 CA 로 확인되지 않음 (또는 만료, 펌웨어 서명 용도 아님)')
    key_ver = publisher_key_version(c)
    if key_ver is None:  # CA 가 발급했어도 제작자 인증서가 아님
        raise SystemExit('[거부] 제작자 인증서가 아님 (이름이 CN=OTA Publisher, OU=key-N 이 아님)')
    print('제작자 인증서 OK (CA 확인, 펌웨어 서명 용도, 이름 확인)')
    if key_ver < my_key_ver:  # 기기가 이미 더 새 키를 봤다 → 옛 키는 폐기된 것
        raise SystemExit(f'[거부] 폐기된 옛 키 : key-{key_ver} (기기가 본 최신 키 : key-{my_key_ver})')
    print(f'키 버전 OK : key-{key_ver}')

    # 2) manifest + 서명 받기 → 서명이 맞아야만 내용을 읽는다
    m, s = work / 'manifest.json', work / 'manifest.json.sig'
    m.write_bytes(get(f'/{target}/manifest.json'))
    s.write_bytes(get(f'/{target}/manifest.json.sig'))
    if not verify_sig(m, s, c):
        raise SystemExit('[위조 의심] manifest 서명 검증 실패')
    print('manifest 서명 OK')
    manifest = json.loads(m.read_bytes())
    err = check_manifest(manifest, MAX_SIZE)
    if err:
        raise SystemExit(f'[거부] manifest 내용 이상 : {err}')
    if manifest['hw'] != my_hw:  # 서명은 맞아도 내 하드웨어용이 아니면 거부
        raise SystemExit(f"[거부] 다른 하드웨어용 펌웨어 : {manifest['hw']} (이 기기 : {my_hw})")
    print('하드웨어 확인 OK :', my_hw)
    if now > manifest['expires']:  # 옛 manifest 를 계속 주는 것 방지
        raise SystemExit(f"[거부] 만료된 manifest : {fmt(manifest['expires'])} 에 만료")
    print('만료 확인 OK :', fmt(manifest['expires']), '까지')
    if manifest['version'] <= my_ver:  # 지금 설치된 것보다 새 버전이어야 함
        raise SystemExit(f"[거부] 새 버전이 아님 : v{manifest['version']} (설치된 버전 : v{my_ver})")
    if manifest['version'] <= bad_ver:  # 부팅에 실패했던 버전은 다시 받지 않음 (청크 받기 전에 멈춤)
        raise SystemExit(f"[건너뜀] v{manifest['version']} 은 부팅에 실패했던 버전 → 더 새 버전을 기다림")
    print(f"버전 확인 OK : v{my_ver} → v{manifest['version']}")

    # 3) 청크 받기 (manifest 의 해시로 검증)
    size, cs = manifest['size'], manifest['chunk_size']
    chunk_dir = work / 'chunks'
    chunk_dir.mkdir(exist_ok=True)

    for i, h in enumerate(manifest['chunk_sha256']):
        f = chunk_dir / f'{i}.bin'
        if f.exists():
            if verify(f.read_bytes(), h):  # 이미 받았고 멀쩡하면 건너뜀 (이어받기)
                continue
            print(f'청크 {i} 저장본이 manifest 와 다름 → 다시 받음')

        start, end = i * cs, min((i + 1) * cs, size) - 1
        f.write_bytes(get_chunk(target, i, start, end, size, h))
        print(f'청크 {i} 받음 (검증 OK)')
    return manifest, key_ver
