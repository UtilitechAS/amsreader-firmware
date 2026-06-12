#!/usr/bin/env python3
import json, os
BASE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "test", "payloads")
rows = json.load(open(f"{BASE}/manifest.json"))

def esc(s): return (s or "").replace('"', '\\"')

def emit(rows):
    out = []
    for r in rows:
        out.append('  {{ "test/payloads/{f}", "{man}", "{src}" }},'.format(
            f=r["file"], man=esc(r["manufacturer"]), src=esc(r["source"])))
    return "\n".join(out)

unenc_ok   = [r for r in rows if not r["encrypted"] and r["expect"] == "ok"]
unenc_edge = [r for r in rows if not r["encrypted"] and r["expect"] == "edge"]
enc_keyed  = [r for r in rows if r["encrypted"] and r.get("ek_secret")]

h = []
h.append("/**")
h.append(" * @copyright Utilitech AS 2023-2026")
h.append(" * License: Fair Source")
h.append(" *")
h.append(" * AUTO-GENERATED from test/payloads/manifest.json by")
h.append(" * scripts/gen_fixtures_header.py — do not edit by hand.")
h.append(" */")
h.append("#ifndef _FIXTURES_GENERATED_H")
h.append("#define _FIXTURES_GENERATED_H")
h.append("")
h.append("struct Fixture { const char* path; const char* manufacturer; const char* source; };")
h.append("struct KeyedFixture { const char* path; const char* ek_secret; const char* ak_secret; };")
h.append("")
h.append(f"// {len(unenc_ok)} unencrypted frames expected to decode to a valid list")
h.append("static const Fixture UNENC_OK[] = {")
h.append(emit(unenc_ok))
h.append("};")
h.append("")
h.append(f"// {len(unenc_edge)} unencrypted but corrupt/truncated/segment — must NOT crash")
h.append("static const Fixture UNENC_EDGE[] = {")
h.append(emit(unenc_edge))
h.append("};")
h.append("")
h.append(f"// {len(enc_keyed)} encrypted frames for which we hold a verified key")
h.append("static const KeyedFixture ENC_KEYED[] = {")
for r in enc_keyed:
    h.append('  {{ "test/payloads/{f}", "{ek}", {ak} }},'.format(
        f=r["file"], ek=r["ek_secret"],
        ak=('"%s"' % r["ak_secret"]) if r.get("ak_secret") else "nullptr"))
h.append("};")
h.append("")
h.append("#endif")
OUT = os.path.join(os.path.dirname(BASE), "test_decoder", "fixtures_generated.h")
open(OUT, "w").write("\n".join(h) + "\n")
print(f"wrote fixtures_generated.h: {len(unenc_ok)} ok, {len(unenc_edge)} edge, {len(enc_keyed)} keyed")
