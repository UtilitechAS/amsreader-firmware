
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


# Getting started building or modifying

## Tools

### Kicad

Install the [KiCad](http://www.kicad-pcb.org/) program to edit the schematic or PCB.
KiCad documentation and forums:

* https://kicad-pcb.org/help/documentation/#_getting_started
* https://forum.kicad.info/
* https://www.reddit.com/r/KiCad/
* https://en.wikipedia.org/wiki/KiCad

### Simulator

If you want to simulate parts of the circuit you also need a simulator. This is
highly recommended! This saves a *lot* of troubleshooting and makes you find
solutions you otherwise would not have found.

[Electronic circuit simulation](https://en.wikipedia.org/wiki/Electronic_circuit_simulation)
using computers have a long history. Many of them have origins directly or indirectly
related to the classic SPICE simulator (e.g. [Ngspice](http://ngspice.sourceforge.net)).
At the core they work similar to source code compilers - you give it a text file
describing the circuit and it produces a textual simulation result. Some of the simulators
are intended to be used just in text mode while other have a graphical frontend where
you are able to draw the circuit like in a schematic editor:

 * [QUCS](http://qucs.sourceforge.net/) - Quite Universal Circuit Simulator.
 * [QUCS-S](https://ra3xdh.github.io/) - A qucs version using ngspice as simulation backend. This one has been used for the simulations for board 3.
 * [eSim](http://esim.fossee.in/).
 * [Other alternatives](https://en.wikipedia.org/wiki/List_of_free_electronics_circuit_simulators).

### Git

While it is possible to download the content from this repository as a compresset
zip file, you want to use git to fetch the content. For Linux install depending on
distribution with

```
apt-get install git   # debian, ubuntu, etc
dnf install git       # fedora
yum install git       # rhel, centos
```

For windows the most convenient option is to install [git for windows](https://git-scm.com/download/win).

To download the source of this repository run:

```
git clone https://github.com/roarfred/AmsToMqttBridge
cd AmsToMqttBridge
git submodule init
git submodule update --recursive
```


