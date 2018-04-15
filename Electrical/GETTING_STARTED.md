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

Make sure that all submodules are fetched. If not `git submodule status` will show something like the following (notice the `-` in front):

```bash
-f886cacb58461d356730e744d9d2ae55590825e4 kicad-symbols
```

while when properly fetched it will show the commit id normally
(plus any labels if appropriate):

```bash
 f886cacb58461d356730e744d9d2ae55590825e4 kicad-symbols (5.0.0-rc1)
```

To mitigate, run update:

```bash
$ git submodule update --init
Submodule 'Electrical/kicad-libs/kicad-symbols' (https://github.com/kicad/kicad-symbols) registered for path 'kicad-symbols'
Cloning into '.../AmsToMqttBridge/Electrical/kicad-libs/kicad-symbols'...
Submodule path 'kicad-symbols': checked out 'f886cacb58461d356730e744d9d2ae55590825e4'
```
