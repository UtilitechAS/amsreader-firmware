
# Overview

This board can be used to simulate a M-Bus master device instead of
connecting to the real AMS unit, i.e. usable for test and development.
It takes a 5 V TTL level input (serial) signal and converts it
to a M-bus level signal. It supports both sending and receiving.
Inspired by [MBus_USB.pdf](https://github.com/rscada/libmbus/blob/master/hardware/MBus_USB.pdf)<sup>1</sup>,
although significantly improved by using zener diodes so that it is
flexible and can work with any external power source between
21 and 42 Volt. Most DC-DC boost/step-up converter
modules<sup>2</sup> should be fine for this.

![](MBus_Simulator.png?raw=true)

## BOM

* 1 x zener diode [13V](https://www.elfadistrelec.no/en/zener-diode-017aa-13-on-semiconductor-1n5350bg/p/17006687) (note below)
* 1 x zener diode [4.7V](https://www.elfadistrelec.no/en/zener-diode-do-35-500-mw-vishay-bzx55c4v7/p/30030851)
* 1 x [BD136 transistor](https://www.elfadistrelec.no/en/power-transistor-to-126-pnp-45-no-brand-bd136-16/p/17130046) (most pnp bjt will probably work (with 40+ operating voltage))
* 4 x [BC337 transistor](https://www.elfadistrelec.no/en/transistor-to-92-npn-45-800-ma-diotec-bc337-25bk/p/30012857) (probably any npn bjt will do (with 40+ operating voltage)
* 1 x resistor 82 ohm
* 1 x resistor 1k
* 1 x resistor 8.2k (you can use 10k if power supply is well above 21V)
* 5 x resistor 10k
* 1 x resistor 220k
* 4 x pin header 1x2


Note for the 13V zener. This board design depends on the zener to
operate below 1mA, e.g. like 1N5350 (figure 8) whereas a BZX55
(figure 9) will maybe not operate properly.

![](zener_ok.png?raw=true) ![](zener_not_ok.png?raw=true)

This limitation could be lifted by reducing the 8.2k resistor so that it
draws enough current.


## Schematic

![](MBus_Simulator.schematic.png?raw=true)
[SVG version](MBus_Simulator.schematic.svg?raw=true)

## PCB

![](MBus_Simulator.B.Cu.png?raw=true)
[SVG version](MBus_Simulator.B.Cu.svg?raw=true)
![](MBus_Simulator.F.Cu.png?raw=true)
[SVG version](MBus_Simulator.F.Cu.svg?raw=true)

## Version history

Rev A - Only transmit. Wrong design, does not work.
Rev B - Both transmit and receive. Should work (not tested yet).

-------------

<sup>1</sup>
See also https://electronics.stackexchange.com/questions/99388/designing-a-m-bus-master-up-to-10-slaves/ and https://electronics.stackexchange.com/a/214477/568.

<sup>2</sup>
Like for instance [this one](http://hobbycomponents.com/power/698-xl60009-dc-dc-step-up-boost-converter) for £3.
