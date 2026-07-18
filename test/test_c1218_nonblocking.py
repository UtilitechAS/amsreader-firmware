from pathlib import Path


source = Path("src/C1218MeterCommunicator.cpp").read_text()
assert "waitByte" not in source
assert "readExact" not in source
assert "delay(" not in source
assert "serviceRequest" in source

abort_cycle = source.split("void C1218MeterCommunicator::abortCycle", 1)[1].split("void C1218MeterCommunicator::discardInput", 1)[0]
assert "sessionOpen && stage != LOGOFF && stage != TERMINATE" in abort_cycle
assert "stage = LOGOFF" in abort_cycle
assert "requestFailed" in source
assert "UART configured: RX=" in source
assert "nextPoll = now + RETRY_DELAY" in source
