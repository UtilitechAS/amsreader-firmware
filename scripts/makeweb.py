import os
import re
import shutil
from css_html_js_minify import html_minify, js_minify, css_minify

webroot = "web"
srcroot = "src/web/root"

version = os.environ.get('GITHUB_TAG')
if version == None:
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
    
    if filename.endswith(".html"):
        content = html_minify(content)
    elif filename.endswith(".css"):
        content = css_minify(content)
    elif filename.endswith(".js") and filename != 'gaugemeter.js':
        content = js_minify(content)

    with open(dstfile, "w") as dst:
        dst.write("const char ")
        dst.write(varname)
        dst.write("[] PROGMEM = R\"==\"==(")
        dst.write(content)
        dst.write(")==\"==\";\n")
        dst.write("const int ");
        dst.write(varname)
        dst.write("_LEN = ");
        dst.write(str(len(content)))
        dst.write(";");
        