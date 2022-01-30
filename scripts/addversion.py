import os
import subprocess

FILENAME_VERSION_H = 'src/version.h'
version = os.environ.get('GITHUB_TAG')
if version == None:
    result = subprocess.run(['git','rev-parse','--short','HEAD'], stdout=subprocess.PIPE)
    version = result.stdout.decode('utf-8').strip()

import datetime

hf = """
#ifndef VERSION
  #define VERSION "{}"
#endif
""".format(version)
with open(FILENAME_VERSION_H, 'w+') as f:
    f.write(hf)
