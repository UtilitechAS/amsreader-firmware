from pathlib import Path


source = Path("src/C1218MeterCommunicator.cpp").read_text()
assert "waitByte" not in source
assert "readExact" not in source
assert "delay(" not in source
assert "delayMicroseconds(175)" in source
assert "serviceRequest" in source

abort_cycle = source.split("void C1218MeterCommunicator::abortCycle", 1)[1].split("void C1218MeterCommunicator::discardInput", 1)[0]
assert "sessionMayBeOpen" in abort_cycle
assert "stage = sessionOpen ? LOGOFF : TERMINATE" in abort_cycle
assert "requestFailed" in source
assert "rejectPacket" in source
assert "MAX_RETRIES" in source
assert "CHANNEL_TIMEOUT" in source
assert "INTERCHAR_TIMEOUT" in source
assert "EXTENDED_TABLE_INTERVAL" in source
assert "readExtendedTable" in source
assert "C1218_SNS" in source
assert "UART configured: RX=" in source
assert "nextPoll = now + RETRY_DELAY" in source
