# Electrical Design

## Explained
The design is using an opamp as a compined level converter and a schmitt trigger, in order to convert 
the HAN signal into a 3.3V compatible serial format. As the input levels on the opamp is much higher 
than our 5V supply, we are rectifying and smoothing the HAN signal to use for a supply here. (VDD)

As a power source, we've used a micro USB connector, providing 5V, just as this is cheap and easy. However,
no part of the circuit will need any other power than the 3.3V, so any combination of power source and 
regulator that provides the 3.3V will do.

The ESP8266 setup is a rather standaraized setup and will allow for programming the ESP directly on the 
board, if needed. During programming, it might be neccessary to disconnect the incoming HAN.

### **!!important changes!!**
> * R8 should not be used
> * The two unused inputs on the LM358 should be grounded (Pin 5 and 6)

## Schematics
![Schematics](./Schematics.PNG)

## PCB
![PCB](./PCB.PNG)

## Componenet list
Name | Value | Part
-----|------|-----
C1|10uF/25V|[P5148-ND](https://www.digikey.no/product-detail/en/panasonic-electronic-components/ECA-1EM100/P5148-ND/245007)
C2|100nF|[BC1101CT-ND](https://www.digikey.no/product-detail/en/vishay-bc-components/K104K10X7RF5UH5/BC2665CT-ND/2356879)
C3|220uF/35V|[1572-1706-ND](https://www.digikey.no/product-detail/en/illinois-capacitor/227CKS035M/1572-1706-ND/5411729)
D1|1N4148|[1N4148FS-ND](https://www.digikey.no/product-detail/en/fairchild-on-semiconductor/1N4148/1N4148FS-ND/458603)
P1||[609-4613-1-ND](https://www.digikey.no/product-detail/en/amphenol-fci/10118192-0001LF/609-4613-1-ND/2785378)
P2|Female|[S7004-ND](https://www.digikey.no/product-detail/en/sullins-connector-solutions/PPTC061LFBN-RC/S7004-ND/810145)
P3|Male|[952-2262-ND](https://www.digikey.no/product-detail/en/harwin-inc/M20-9990246/952-2262-ND/3728226)
P4||[609-1047-ND](https://www.digikey.no/product-detail/en/amphenol-fci/54602-910LF/609-1047-ND/1001361)
Q1|BC337|[BC33725TACT-ND](https://www.digikey.no/product-detail/en/fairchild-on-semiconductor/BC33725TA/BC33725TACT-ND/1532787)
R1-5,10,11|10K|[CF14JT10K0CT-ND](https://www.digikey.no/product-detail/en/stackpole-electronics-inc/CF14JT10K0/CF14JT10K0CT-ND/1830374)
R8|100K|[CF14JT100KCT-ND](https://www.digikey.no/product-detail/en/stackpole-electronics-inc/CF14JT100K/CF14JT100KCT-ND/1830399)
R9|330R|[CF14JT330RCT-ND](https://www.digikey.no/product-detail/en/stackpole-electronics-inc/CF14JT330R/CF14JT330RCT-ND/1830338)
R12,13|47K|[CF14JT47K0CT-ND](https://www.digikey.no/product-detail/en/stackpole-electronics-inc/CF14JT47K0/CF14JT47K0CT-ND/1830391)
S1,2||[450-1650-ND](https://www.digikey.no/product-detail/en/te-connectivity-alcoswitch-switches/1825910-6/450-1650-ND/1632536)
U2|LM1117-3.3|[LM1117IMPX-3.3/NOPBCT-ND](https://www.digikey.no/product-detail/en/texas-instruments/LM1117IMPX-3.3-NOPB/LM1117IMPX-3.3-NOPBCT-ND/3440160)
U3|LM358|[LM358NGOS-ND](https://www.digikey.no/product-detail/en/on-semiconductor/LM358NG/LM358NGOS-ND/918519)

Note: The ESP8266 is not included here. It can easily be found on ebay etc
