import serial
import time
import sys

import sample_data

serial_port = sys.argv[1]

with serial.Serial(port=serial_port, baudrate=2400, bytesize=8, parity='E', stopbits=1) as ser:
    time.sleep(1)
    while True:
        for packet in sample_data.kaifa_20190508_packets[:]:
            sys.stdout.flush()
            ser.write(packet)
            print('sleeping')
            time.sleep(2)
        break


# POST  HTTP/1.1
# Host: kanskje.de:8181
# User-Agent: ESP32HTTPClient
# Connection: close
# Accept-Encoding: identity;q=1,chunked;q=0.1,*;q=0
# Authorization: Basic dmVnYXJ3ZTpmaXNrZW4=
# Content-Type: application/json
# Content-Length: 44
# 
# {"up":88695,"t":1557354754,"data":{"P":931}}

# POST / HTTP/1.1
# Host: kanskje.de:8181
# User-Agent: curl/7.64.0
# Accept: */*
# Content-Type: application/json
# Content-Length: 44
# 
# {"up":88695,"t":1557354754,"data":{"P":931}}
