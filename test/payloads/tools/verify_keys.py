#!/usr/bin/env python3
"""Verify that each decryption key still turns its payload into valid DLMS.

Reads keys/keymap.json (payload -> secret names) and resolves the actual key
values from either the environment (CI / GitHub secrets) or, as a local
fallback, keys/keys.local.json.

Usage:  python3 tools/verify_keys.py
Exit 0 if every mapped payload decrypts to a plausible DLMS APDU.

Decryption follows src/decoder/src/GcmParser.cpp:
  IV  = system_title(8) || frame_counter(4)
  GCM keystream = AES-CTR with initial counter = IV || 00000002
  (tag verification is skipped here — we only confirm the EK produces sane data)
"""
import json, os, re, sys
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)  # test/payloads


def load_key(name):
    if name is None:
        return None
    if name in os.environ:
        return bytes.fromhex(os.environ[name])
    local = os.path.join(ROOT, "keys", "keys.local.json")
    if os.path.exists(local):
        d = json.load(open(local))
        if name in d:
            return bytes.fromhex(d[name])
    return None


def read_hex(path):
    return bytes.fromhex(re.sub(r"[^0-9a-fA-F]", "", open(path).read()))


def decrypt(frame, ek):
    i = frame.find(0xDB)  # GCM security header tag
    if i < 0:
        raise ValueError("no 0xDB GCM header found")
    p = i + 1
    stl = frame[p]; p += 1
    st = frame[p:p + stl]; p += stl
    L = frame[p]; p += 1
    if L in (0x81, 0x82, 0x84):
        n = {0x81: 1, 0x82: 2, 0x84: 4}[L]
        L = int.from_bytes(frame[p:p + n], "big"); p += n
    sec = frame[p]; p += 1
    fc = frame[p:p + 4]; p += 4
    iv = st + fc
    authd = bool(sec & 0x10)
    ctlen = L - (12 if authd else 0) - 5
    ct = frame[p:p + ctlen]
    ctr0 = iv + b"\x00\x00\x00\x02"
    dec = Cipher(algorithms.AES(ek), modes.CTR(ctr0)).decryptor()
    return dec.update(ct) + dec.finalize()


def plausible_dlms(pt):
    # data-notification (0x0F) or general APDU tags commonly seen after decrypt
    return len(pt) > 8 and pt[0] in (0x0F, 0xE6, 0x09, 0x01, 0xC2)


def main():
    keymap = json.load(open(os.path.join(ROOT, "keys", "keymap.json")))
    ok = bad = skip = 0
    for m in keymap:
        ek = load_key(m["ek_secret"])
        path = os.path.join(ROOT, m["payload"])
        if ek is None:
            print(f"SKIP  {m['payload']}  (key {m['ek_secret']} not available)")
            skip += 1
            continue
        try:
            pt = decrypt(read_hex(path), ek)
            if plausible_dlms(pt):
                print(f"OK    {m['payload']}  -> DLMS APDU tag 0x{pt[0]:02X}, {len(pt)} bytes")
                ok += 1
            else:
                print(f"FAIL  {m['payload']}  -> implausible plaintext {pt[:8].hex()}")
                bad += 1
        except Exception as e:
            print(f"FAIL  {m['payload']}  -> {e}")
            bad += 1
    print(f"\n{ok} ok, {bad} failed, {skip} skipped")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
