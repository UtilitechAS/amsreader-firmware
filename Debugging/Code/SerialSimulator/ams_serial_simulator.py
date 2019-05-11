import serial
import time
import sys

import sample_data

with serial.Serial(port='COM15', baudrate=2400, bytesize=8, parity='E', stopbits=1) as ser:
    for packet in sample_data.kaifa_20190508_packets:
        print('sleeping')
        sys.stdout.flush()
        time.sleep(2)
        ser.write(packet)

