import os
import subprocess

FILENAME_VERSION_H = 'src/version.h'
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

import datetime

hf = """
#ifndef VERSION
  #define VERSION "{}"
#endif
""".format(version)
with open(FILENAME_VERSION_H, 'w+') as f:
    f.write(hf)
