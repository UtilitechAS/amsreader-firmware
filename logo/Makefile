
ALL = bridge.png network-transmit-receive.png pcb.png powerlines.png simple_meter_icon.png

%.png: %.svg
	inkscape --export-height=100 $^ --export-png=$@

all: $(ALL)

clean:
	rm -rf $(ALL)


