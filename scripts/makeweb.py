import os
import re
import shutil

webroot = "web"
srcroot = "src/web/root"


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

    with open(dstfile, "w") as dst:
        dst.write("const char ")
        dst.write(varname)
        dst.write("[] PROGMEM = R\"==\"==(\n")
        with open(srcfile, "r") as src:
            for line in src.readlines():
                dst.write(line)
        dst.write("\n)==\"==\";\n")