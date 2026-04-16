"""
Unified pre-build script: embeds web UI assets, webserver JSON, and MQTT handler
JSON templates as C PROGMEM byte arrays.

Asset sources (in priority order — custom/ overrides defaults):
  UI dist:         custom/ui/dist/   or  ui/dist/
  Webserver JSON:  custom/ui/json/   or  src/webserver/json/
  MQTT JSON:       src/mqtt/json/

Generated headers:
  src/webserver/html/*.h   — gzip-compressed UI JS/CSS; raw others
  src/mqtt/json/*.h        — MQTT handler JSON templates (raw string literals)
"""

import os
import re
import shutil
import subprocess
import gzip

try:
    from css_html_js_minify import html_minify, js_minify, css_minify
except ImportError:
    try:
        from SCons.Script import DefaultEnvironment
        env = DefaultEnvironment()
        env.Execute(
            env.VerboseAction(
                '$PYTHONEXE -m pip install "css_html_js_minify"',
                "Installing css_html_js_minify",
            )
        )
        from css_html_js_minify import html_minify, js_minify, css_minify
    except Exception:
        print("WARN: css_html_js_minify unavailable — skipping minification")
        html_minify = js_minify = css_minify = lambda x: x


def get_version():
    version = os.environ.get("GITHUB_TAG")
    if version:
        return version
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True, check=False
        )
        if result.returncode == 0:
            return result.stdout.decode("utf-8").strip()
    except Exception:
        pass
    return "SNAPSHOT"


def clean_or_create(path):
    if os.path.exists(path):
        shutil.rmtree(path)
    os.makedirs(path)


def varname(filename):
    return re.sub(r"[^0-9a-zA-Z]+", "_", filename).upper()


def write_progmem_gzip(dstfile, vname, content_bytes):
    """Write gzip-compressed content as a PROGMEM byte array."""
    compressed = gzip.compress(content_bytes, compresslevel=9)
    with open(dstfile, "w") as f:
        f.write(f"static const char {vname}[] PROGMEM = {{")
        f.write(", ".join(str(c) for c in compressed))
        f.write("};\n")
        f.write(f"const int {vname}_LEN PROGMEM = {len(compressed)};")


def write_progmem_raw(dstfile, vname, content_bytes):
    """Write null-terminated content as a PROGMEM byte array."""
    data = content_bytes + b"\0"
    with open(dstfile, "w") as f:
        f.write(f"static const char {vname}[] PROGMEM = {{")
        f.write(", ".join(str(c) for c in data))
        f.write("};\n")
        f.write(f"const int {vname}_LEN PROGMEM = {len(content_bytes)};")


def write_progmem_raw_string(dstfile, vname, content):
    """Write content as a PROGMEM raw string literal (no compression)."""
    with open(dstfile, "w") as f:
        f.write(f'static const char {vname}[] PROGMEM = R"==(')
        f.write(content)
        f.write(')==\";\n')
        f.write(f"const int {vname}_LEN PROGMEM = {len(content)};")


# ---------------------------------------------------------------------------
# 1. Web UI (gzip-compressed dist + raw webserver JSON)
# ---------------------------------------------------------------------------

version = get_version()

ui_dist   = "custom/ui/dist"   if os.path.exists("custom/ui/dist")   else "ui/dist"
ui_json   = "custom/ui/json"   if os.path.exists("custom/ui/json")   else "src/webserver/json"
ui_out    = "src/webserver/html"

if os.path.exists(ui_dist):
    clean_or_create(ui_out)

    for webroot in [ui_dist, ui_json]:
        if not os.path.exists(webroot):
            continue
        for filename in os.listdir(webroot):
            if os.path.isdir(os.path.join(webroot, filename)):
                continue
            basename = re.sub(r"[^0-9a-zA-Z]+", "_", filename)
            srcfile  = os.path.join(webroot, filename)
            dstfile  = os.path.join(ui_out, basename + ".h")
            vname    = basename.upper()

            with open(srcfile, encoding="utf-8") as f:
                content = f.read()
                content = content.replace("/index.js",  f"index-{version}.js")
                content = content.replace("/index.css", f"index-{version}.css")

            try:
                if filename.endswith(".html"):
                    content = html_minify(content)
                elif filename.endswith(".json") and webroot == ui_json:
                    content = js_minify(content)
            except Exception:
                print(f"WARN: minification failed for {filename}")

            content_bytes = content.encode("utf-8")

            # JS and CSS are gzip-compressed (served with Content-Encoding: gzip)
            if filename in ("index.js", "index.css"):
                write_progmem_gzip(dstfile, vname, content_bytes)
            else:
                write_progmem_raw(dstfile, vname, content_bytes)
else:
    print(f"WARN: UI dist not found at '{ui_dist}' — skipping UI header generation. Run 'npm run build' in ui/")

# ---------------------------------------------------------------------------
# 2. MQTT handler JSON templates (raw string literals, no compression)
# ---------------------------------------------------------------------------

mqtt_json_src = "src/mqtt/json"
mqtt_json_out = "src/mqtt/json"   # headers live alongside the JSON sources

for filename in os.listdir(mqtt_json_src):
    if not filename.endswith(".json"):
        continue
    basename = re.sub(r"[^0-9a-zA-Z]+", "_", filename)
    srcfile  = os.path.join(mqtt_json_src, filename)
    dstfile  = os.path.join(mqtt_json_out, basename + ".h")
    vname    = basename.upper()

    with open(srcfile, encoding="utf-8") as f:
        content = f.read().replace("${version}", version)

    try:
        if filename.endswith(".json"):
            content = js_minify(content)
    except Exception:
        print(f"WARN: minification failed for {filename}")

    write_progmem_raw_string(dstfile, vname, content)
