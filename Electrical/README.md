
# Overview

Here are links to the different board designs together with a short summary.

## Board 1

The project's original [board design](Board_001). It

* is based on the ESP8266 chip.
* is powered by USB.
* uses a very simple voltage divider to demodulate the M-bus signal.
* has shematic and pcb design only available as finished pdf/png files.

### Status

Prototypes have been made and some people have started using them(?).

## Board 2

This [board design](Board_002) is a newer alternative to the original. It

* is an Arduino shield.
* uses the industry standard TSS721 chip to interface the M-bus.
* is optically isolated.
* has shematic and pcb design available in editable [KiCad](http://www.kicad-pcb.org/) source files.

### Status

Unfinished, just started.

## Board 3

This [board](Board_003) is a M-bus master simulator to be able to develop and
test the other boards without being dependent on having and using a real
AMS unit.

### Status

Implementation done.
