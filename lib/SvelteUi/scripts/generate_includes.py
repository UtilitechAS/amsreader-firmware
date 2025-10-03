import os
import re
import shutil
import subprocess
import gzip

# Attempt to import minifiers
try:
    from css_html_js_minify import html_minify, js_minify, css_minify
except ImportError:
    html_minify = lambda x: x
    js_minify = lambda x: x
    css_minify = lambda x: x
    print("WARN: Minifiers not installed, files will not be minified.")

# Root folder for generated headers
srcroot = "lib/SvelteUi/include/html"

# Determine version
version = os.environ.get('GITHUB_TAG')
if version is None:
    try:
        result = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, check=False)
        version = result.stdout.decode('utf-8').strip() if result.returncode == 0 else "SNAPSHOT"
    except:
        version = "SNAPSHOT"

# Ensure clean include folder
if os.path.exists(srcroot):
    shutil.rmtree(srcroot)
os.makedirs(srcroot, exist_ok=True)

# Folders to scan
webroots = ["lib/SvelteUi/app/dist", "lib/SvelteUi/json"]

for webroot in webroots:
    if not os.path.exists(webroot):
        print(f"WARN: Folder not found: {webroot}")
        continue

    for filename in os.listdir(webroot):
        basename = re.sub("[^0-9a-zA-Z]+", "_", filename)
        srcfile = os.path.join(webroot, filename)
        dstfile = os.path.join(srcroot, basename + ".h")
        varname = basename.upper()

        # Read file content
        with open(srcfile, encoding="utf-8") as f:
            content = f.read()
            # Replace references to JS/CSS with versioned filenames
            content = content.replace("/index.js", f"index-{version}.js")
            content = content.replace("/index.css", f"index-{version}.css")

        # Minify if possible
        try:
            if filename.endswith(".html"):
                content = html_minify(content)
            elif filename.endswith(".json"):
                content = js_minify(content)
            elif filename.endswith(".css"):
                content = css_minify(content)
            elif filename.endswith(".js"):
                # JS6+ may break normal minifier, skip or handle later
                pass
        except Exception as e:
            print(f"WARN: Minify failed for {filename}: {e}")

        # Encode to bytes for PROGMEM
        content_bytes = content.encode("utf-8")
        compress = filename.endswith((".js", ".css"))  # Compress JS/CSS only
        if compress:
            content_bytes = gzip.compress(content_bytes, compresslevel=9)
            content_len = len(content_bytes)
        else:
            content_len = len(content_bytes)
            content_bytes += b"\0"  # Null-terminate for C strings

        # Write header file
        with open(dstfile, "w") as dst:
            dst.write(f"static const char {varname}[] PROGMEM = {{")
            dst.write(", ".join(str(c) for c in content_bytes))
            dst.write("};\n")
            dst.write(f"const int {varname}_LEN PROGMEM = {content_len};\n")
