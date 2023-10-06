# esp32_garagedoor_iot
An IoT project to secure the ancient garage door motor with an ultrasonic sensor that checks for objects in front of the door. 
## Description
The ESP32 (I am using an ESP32-WROOM-32E) looks for an object in font of the gargae door by using the JSN SR04T ultrasonic sensor.
If the sensor reports an object closer than a given distance, a relay is switched to open. This blocks the garage door motor which expects an NC switch.
Furthermore, the ESP32 connects to a (currently) preconfigured WiFi where it sets up a simple webserver in order to make its values readble. Each variable is printed out indivudually as plain text under its own url and all together in a json response which makes it easy to use them in smart home apps (e.g. Homebridge). There is also one option to manually overwrite the sensor readings by set car = true using a simple HTTP_GET request. 
## Hardware
- todo: pictures and fritzings of the wiring, explanation of the setup
## Configuration
There is currently no way other to configure this device than by compling flashing a new binary onto it. you must supply your local WiFi credentials in wifi_creds.h and define the threshold for your garage door as limit. Limit is the minimum distance between any given object (e.g. a car) in front of your garage door for not to get it by it when it is opened.
