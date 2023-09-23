import os
import re
import shutil
import subprocess
import gzip

try:
    from css_html_js_minify import html_minify, js_minify, css_minify
except:
    from SCons.Script import (
        ARGUMENTS,
        COMMAND_LINE_TARGETS,
        DefaultEnvironment,
    )
    env = DefaultEnvironment()

    env.Execute(
        env.VerboseAction(
            '$PYTHONEXE -m pip install "css_html_js_minify" ',
            "Installing Python dependencies",
        )
    )
    try:
        from css_html_js_minify import html_minify, js_minify, css_minify
    except:
        print("WARN: Unable to load minifier")


srcroot = "lib/SvelteUi/include/html"

version = os.environ.get('GITHUB_TAG')
if version == None:
    try:
      result = subprocess.run(['git','rev-parse','--short','HEAD'], capture_output=True, check=False)
      if result.returncode == 0:
        version = result.stdout.decode('utf-8').strip()
      else:
        version = "SNAPSHOT"
    except:
      version = "SNAPSHOT"

if os.path.exists(srcroot):
    shutil.rmtree(srcroot)
    os.mkdir(srcroot)
else:
    os.mkdir(srcroot)

for webroot in ["lib/SvelteUi/app/dist", "lib/SvelteUi/json"]:
    for filename in os.listdir(webroot):
        basename = re.sub("[^0-9a-zA-Z]+", "_", filename)

        srcfile = webroot + "/" + filename
        dstfile = srcroot + "/" + basename + ".h"

        varname = basename.upper()

        with open(srcfile, encoding="utf-8") as f:
            content = f.read()
            content = content.replace("index.js", "index-"+version+".js")
            content = content.replace("index.css", "index-"+version+".css")

        try:    
            if filename.endswith(".html"):
                content = html_minify(content)
            elif filename.endswith(".json"):
                content = js_minify(content)
        except:
            print("WARN: Unable to minify")
        
        content_bytes = content.encode("utf-8")
        if filename in ["index.js", "index.css"]:
            content_bytes = gzip.compress(content_bytes, compresslevel=9)
            content_len = len(content_bytes)
        else:
            content_len = len(content_bytes)
            content_bytes += b"\0"
        
        with open(dstfile, "w") as dst:
            dst.write("static const char ")
            dst.write(varname)
            dst.write("[] PROGMEM = {")
            dst.write(", ".join([str(c) for c in content_bytes]))
            dst.write("};\n")
            dst.write("const int ");
            dst.write(varname)
            dst.write("_LEN PROGMEM = ");
            dst.write(str(content_len))
            dst.write(";");
            