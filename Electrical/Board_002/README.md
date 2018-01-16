
# Overview

This represents an alernative PCB for interfacing the HAN M-bus connector
on AMS meters.

The biggest difference between this board and [the original board](../Board_001)
is that this board supports communication over a physical wire.

So you can for instance use this board together with an arduino to make
a robust watt-meter display that will never fail because of connection
issues (unlike wifi).

This solution uses the industry standard chip for M-bus communication,
[TSS721A](http://www.ti.com/lit/ds/symlink/tss721a.pdf) with full
galvanic isolation to the bus, e.g. connecting a cable from this PCB
to a mains connected PC for instance will not result in a [ground
loop](https://en.wikipedia.org/wiki/Ground_loop_%28electricity%29).

The schematic and drawings are made with
[KiCad](http://www.kicad-pcb.org/).


# Scope

## Software

Reuse existing [code](../../Code).

## Hardware

Only support receiving transmitted serial data from the bus since the
Kamstrup AMS meters have no physical support for receiving anything.

The primary use case is to be a shield on an Arduino board (e.g. Uno or
Leonardo). A standalone ATtiny version would be nice to have as well.

Support daisy-chaining multiple units on the bus.


# Status

This is unfinished work, currently just started.

# Future

First phase is only supporting converting M-Bus data to serial I/O.
Support for ethernet or USB will be implicit via Arduino/shields. Later
standalone versions with ATTiny will be most useful by including
ethernet/usb support directly.

## Step 1

Create a simple M-Bus master simulator (transmit only) that takes
serial data and modulates onto the bus. Write arduino code to send a
test pattern and verify on oscilloscope.

https://electronics.stackexchange.com/questions/99388/designing-a-m-bus-master-up-to-10-slaves/99390#99390
https://electronics.stackexchange.com/a/214477/568

## Step 2

Connect the TSS721A chip with all required components with a optocoupler
between it and Arduino (e.g. M-Bus to TTL conversion). Connect with the
simulator from step 1 and verify that received data equals the data sendt.

Possibly also test with off-the-shelf M-Bus to TTL conversion hardware
for comparison.

## Step 3

Finish PCB design to be a usable Arduino shield. Integrate the
AmsToMqttBridge software. Display received data on LCD.

## Step 4

Put code onto ATTiny on a standalone board with support for either USB
or ethernet.

The MCP2221 is an inexpensive<sup>1</sup> USB to serial chip that people
[seems](https://www.element14.com/community/groups/open-source-hardware/blog/2016/02/01/implementing-non-ftdi-usb-to-uart-serial-interfaces)
[very](http://www.eevblog.com/forum/reviews/alternatives-to-ftdi-usb-to-uart-converter/)
[satisfied](https://hackaday.io/project/18845-usb-serial-uart)
[with](https://hackaday.com/2016/03/04/dual-uarti2c-breakout-goes-both-ways/).
It does [not](http://blog.zakkemble.co.uk/mcp2221-hid-library/)
[achieve](https://hackaday.com/2017/05/31/counterfeit-hardware-may-lead-to-malware-and-failure/#comment-3636318)
the maximum speeds is theoretically supports, but that is irrelevant for
this project. This board should then be powered by USB, both in that that
is the most natural and convinient choice and that if not then apparently
MCP2221 needs some additional electronics in order to [work properly as
an USB device](https://electronics.stackexchange.com/a/323551/568).


-------------------

<sup>1</sup> Less than 20 NOK at [Elfa
Distrelec](https://www.elfadistrelec.no/en/interface-ic-uart-usb-so-14-microchip-mcp2221-sl/p/11087556),
cheaper elsewhere.

