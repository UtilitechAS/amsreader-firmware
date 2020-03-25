import os

FILENAME_VERSION_H = 'src/version.h'
version = os.environ.get('GITHUB_TAG')
if version == None:
    version = "SNAPSHOT"

import datetime

hf = """
#ifndef VERSION
  #define VERSION "{}"
#endif
""".format(version)
with open(FILENAME_VERSION_H, 'w+') as f:
    f.write(hf)
