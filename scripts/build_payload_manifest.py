#!/usr/bin/env python3
"""Reconstruct a consolidated machine-readable fixtures manifest from the
committed per-manufacturer README tables under test/payloads/."""
import glob, re, json, os

BASE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "test", "payloads")
EDGE = re.compile(r"corrupt|truncat|overflow|garbl|wrong parity|wrong; correct|"
                  r"malformed|partial|misalign|mis-sampl|not (?:usable|cleanly)|"
                  r"negative|edge-case|implausible|clipped|cut short|overwritten",
                  re.I)

rows = []
for f in sorted(glob.glob(f"{BASE}/*/README.md")):
    man = f.split("/")[-2]
    for line in open(f):
        if not line.startswith("| `"):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 7:
            continue
        filecell, src, model, country, proto, enc, notes = cells[:7]
        fn = filecell.replace("`", "").replace("🔑", "").strip()
        if not re.search(r"\.(hex|txt)$", fn):
            continue
        encrypted = "🔒" in enc
        edge = bool(EDGE.search(notes))
        rows.append({
            "file": f"{man}/{fn}",
            "manufacturer": man,
            "source": re.sub(r"\]\(.*?\)", "", src).strip("[]") or src,
            "model": model, "country": country, "protocol": proto,
            "format": "txt" if fn.endswith(".txt") else "hex",
            "encrypted": encrypted,
            "expect": "edge" if edge else "ok",
        })

# attach known decryption-key secret names from keymap.json
keymap = {m["payload"]: m for m in json.load(open(f"{BASE}/keys/keymap.json"))}
for r in rows:
    if r["file"] in keymap:
        r["ek_secret"] = keymap[r["file"]]["ek_secret"]
        r["ak_secret"] = keymap[r["file"]]["ak_secret"]

json.dump(rows, open(f"{BASE}/manifest.json", "w"), indent=1)

# summary
from collections import Counter
print("total fixtures:", len(rows))
def c(pred): return sum(1 for r in rows if pred(r))
print("  unencrypted ok   :", c(lambda r: not r["encrypted"] and r["expect"]=="ok"))
print("  unencrypted edge :", c(lambda r: not r["encrypted"] and r["expect"]=="edge"))
print("  encrypted total  :", c(lambda r: r["encrypted"]))
print("  encrypted w/ key :", c(lambda r: r["encrypted"] and r.get("ek_secret")))
print("  .txt (DSMR)      :", c(lambda r: r["format"]=="txt"))
print("\nunencrypted-ok by manufacturer:")
for k,v in Counter(r["manufacturer"] for r in rows if not r["encrypted"] and r["expect"]=="ok").most_common():
    print(f"   {v:3} {k}")
print("\nedge (unencrypted) files:")
for r in rows:
    if not r["encrypted"] and r["expect"]=="edge":
        print("   ", r["file"])
