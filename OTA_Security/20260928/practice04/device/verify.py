# [기기] 해시 검증 + 제작자 인증서 확인 + manifest 서명 검증
import re, hashlib, subprocess

CA = 'device/trust/ca.crt'  # 출하 때 넣어둔 Root CA 인증서 (기기는 이것만 믿음)


def verify(data, expected_sha256):
    return hashlib.sha256(data).hexdigest() == expected_sha256


def verify_cert(cert, now=None):
    # CA 도장 + 용도가 "펌웨어 서명(codeSigning)" 인가 + now 시각에 유효기간 안인가 (now 없으면 시간 확인 안 함)
    time_opt = ['-attime', str(now)] if now else ['-no_check_time']
    r = subprocess.run(['openssl', 'verify', '-purpose', 'codesign', *time_opt, '-CAfile', CA, cert], capture_output=True)
    return r.returncode == 0


def publisher_key_version(cert):  # 이름이 정확히 "CN=OTA Publisher, OU=key-N" 이면 N, 아니면 None
    subj = subprocess.run(['openssl', 'x509', '-in', cert, '-noout', '-subject'],
                          capture_output=True, text=True).stdout.strip()
    m = re.fullmatch(r'subject=CN=OTA Publisher, OU=key-(\d+)', subj)
    return int(m[1]) if m else None


def verify_sig(file, sig_file, cert):  # 인증서 안의 제작자 공개키로 서명 확인
    r = subprocess.run(['openssl', 'pkeyutl', '-verify', '-certin', '-inkey', cert,
                        '-rawin', '-in', file, '-sigfile', sig_file], capture_output=True)
    return r.returncode == 0


def check_manifest(m, max_size):  # 서명은 맞아도 내용이 앞뒤가 맞는지 (제작자 실수·이상한 값 대비)
    size, cs, n = m['size'], m['chunk_size'], len(m['chunk_sha256'])
    if not 0 < size <= max_size:
        return f'크기 {size} 가 허용 범위(1 ~ {max_size}) 밖'
    if cs <= 0 or n != (size + cs - 1) // cs:
        return f'청크 수 {n} 가 크기 {size} / 청크 {cs} 와 안 맞음'
    return None
