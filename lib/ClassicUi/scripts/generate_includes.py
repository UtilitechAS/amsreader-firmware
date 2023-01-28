import os
import re
import shutil
import subprocess

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


webroot = "lib/ClassicUi/html"
srcroot = "lib/ClassicUi/include/root"

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

for filename in os.listdir(webroot):
    basename = re.sub("[^0-9a-zA-Z]+", "_", filename)

    srcfile = webroot + "/" + filename
    dstfile = srcroot + "/" + basename + ".h"

    varname = basename.upper()

    with open(srcfile, encoding="utf-8") as f:
        content = f.read().replace("${version}", version)

    try:    
        if filename.endswith(".html"):
            content = html_minify(content)
        elif filename.endswith(".css"):
            content = css_minify(content)
        elif (filename.endswith(".js") and filename != 'gaugemeter.js') or filename.endswith(".json"):
            content = js_minify(content)
    except:
        print("WARN: Unable to minify")

    with open(dstfile, "w") as dst:
        dst.write("static const char ")
        dst.write(varname)
        dst.write("[] PROGMEM = R\"==\"==(")
        dst.write(content)
        dst.write(")==\"==\";\n")
        dst.write("const int ");
        dst.write(varname)
        dst.write("_LEN PROGMEM = ");
        dst.write(str(len(content)))
        dst.write(";");
        