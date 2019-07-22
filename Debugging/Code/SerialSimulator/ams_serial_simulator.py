import serial
import time
import sys

import sample_data

serial_port = sys.argv[1]

with serial.Serial(port=serial_port, baudrate=2400, bytesize=8, parity='E', stopbits=1) as ser:
    print(ser)
    print('sleeping')
    time.sleep(1)
    while True:
        for packet in sample_data.kaifa_20190508_packets[:]:
            sys.stdout.flush()
            ser.write(packet)
            print('sleeping')
            time.sleep(2)
        break
