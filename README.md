# esp32_garagedoor_iot
An IoT project to secure the ancient garage door motor with an ultrasonic sensor that checks for objects in front of the door. 
## Description
The ESP32 (I am using an ESP32-WROOM-32E) looks for an object in font of the garage door by using the JSN SR04T ultrasonic sensor.
If the sensor reports an object closer than a given distance, a relay is switched to open. This blocks the garage door motor which expects an NC switch.
Furthermore, the ESP32 connects to a (currently) preconfigured WiFi where it sets up a simple webserver in order to make its values readble. Each variable is printed out indivudually as plain text under its own url and all together in a json response which makes it easy to use them in smart home apps (e.g. Homebridge). There is also one option to manually overwrite the sensor readings by set car = true using a simple HTTP_GET request.
If you do not have access to the WiFi and/or the webserver of the ESP32, the hardware button on the board can be used to overwrite the car sensoring: A long-press toggles the relay and a short one toggles manual_car.

## Hardware Required
- an ESP32. Any other MCU will work as well. It needs to be capable of powering the ultrasonic sensor board with 5 V DC (otherwise you need an external powesupply for that) and at least four GPIO pins: 1 for the relais, one for the hal sensor and two for theJSN SR04T ultrasonic sensor. Furthermore, the binary is about 760900 bytes, so you need at least 761 kbyte of storage available on your MCU. All ESP32 do have more than that (all Espressif-Boards come with at least 4 MiB flash memory).

I found it very easy to work with an ESP32-WROOM-32E board that is already equipped with two relais and a LED on board, as well as a power supply circuit that allows this neat inexpensive device to be powered with 7 - 60 VDC or 5 VDC via terminal connectors. You cn even use dupon wires to power with 5 V the board while progrmaming it via USB/UART.
- some wires and soldering lead. Optional: dupon connectors
- JSN SR04T hal sensor
- 10 k Ohm resistor
## Hardware Setup
- todo: pictures and fritzings of the wiring, explanation of the setup, why a pulldown resistor, etc.
## Configuration
There is currently no way other to configure this device than by compling flashing a new binary onto it. you must supply your local WiFi credentials in wifi_creds.h and define the threshold for your garage door as limit. Limit is the minimum distance between any given object (e.g. a car) in front of your garage door for not to get it by it when it is opened.
