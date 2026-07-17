from pathlib import Path


source = Path("src/C1218MeterCommunicator.cpp").read_text()
assert "waitByte" not in source
assert "readExact" not in source
assert "delay(" not in source
assert "serviceRequest" in source
