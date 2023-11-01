// ESP32 Garage door & car sensoric IoT
// 2023, by Yves Jeanrenaud. Licenced under GNU GPL-3. No warranty nor liability for anything.
// Please refer to https://github.com/yjeanrenaud/esp32_garagedoor_iot/ for more details on this project.
#include <WiFi.h>
#include <WebServer.h>
#include "wifi_creds.h"
WebServer server(80);

//board specifics
#define button 0
#define relay1 16
#define relay2 17
#define led 23
//parking sensor hardware JSN-SR04T ultrasonic doppler
#define trig 14  //33  //Pin 33 trigger Tx
#define echo 13  //32  //Pin 32 echo Rx

#define limit 101  //in cm //hardcoded distance for the garage door todo: editable via web interface
#define diff 2 //±

//reed sensor based garage door sensor
#define reedsensor 22
int reedCurrentState;      // the current reading from the input pin
bool garagedoorOpened = true;   //the state of the door derived from reedCurrentState

void trigPulse() {
  digitalWrite(trig, HIGH);  //Trigger Pulse HIGH
  delayMicroseconds(10);     //for 10 micro seconds
  digitalWrite(trig, LOW);   //Trigger Pulse LOW
}

float pulse;              //echo time duration
float dist_cm;            //distance in cm
bool car = false;         //is there a car
bool manual_car = false;  //is there a manual overwrite

bool relay1state = false; //NO = open, NC = closed
bool relay2state = false; //NO = open, NC = closed
bool ledstate = false; // off

long buttonTimer = 0;
long longPressTime = 250;

bool buttonActive = false;
bool longPressActive = false;
bool hardware_overwride = false;

void setup() {
  // put your setup code here, to run once:
  //Relais: GPIO 16, GPIO 17; ac 250 V dc 30v
  //Taster GPIO 0, LED 23
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(button, INPUT);
  pinMode(led, OUTPUT);

  digitalWrite(button, HIGH);  //internal pullup enabled
  digitalWrite(relay1, false);
  digitalWrite(relay2, false);
  digitalWrite(led, false);


  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  digitalWrite(trig, LOW);
  pinMode(reedsensor, INPUT);
  //digitalWrite(reedsensor, HIGH); //we use a hardware pullup resistor. that cable is too long for other solutions

  Serial.begin(115200);
  //before we set up wifi, let's OPEN the relay1. we use the NO-connector for security
  //because when the ESP32 is dead, the relay is open hence the garage door won't open
  digitalWrite(relay1, LOW);
  Serial.println("relay is open");

  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP-Address of ESP32-WROOM-32E module: ");
  Serial.println(WiFi.localIP());

  //register url handlers as different servers. yes, we got heaps of resources to waste on this mcu
  server.on("/", handleRoot);
  server.on("/dist_cm", handleDistcm);
  server.on("/relay1state", handleRelay1state);
  server.on("/relay2state", handleRelay2state);
  server.on("/car", handleCar);
  server.on("/garagedoor", handleGaragedoor);
  server.on("/json", handleJson);
  server.on("/manual_car", handleManualcar);
  server.on("/manual_reset", handleManualreset);

  server.begin();
  Serial.println("set up finished");
}  // end of setup

void handleRoot() {
  String message = "<h1>ESP32 Garage door &amp; car sensoric IoT</h1>\r\n";
  message += "<p>This is the status page of the IoT parking space and garage door sensor.<br/>\r\n";
  message += "The ESP32 looks for an object in font of the garage door by using the JSN SR04T ultrasonic sensor.<br/>\r\n";
  message += "If the sensor reports an object closer than <strong><i>" + String(limit) + " cm</i></strong> (&plusmn;"+diff+"), the relay 1 is switched to open. This blocks the garage door motor which expects an NC switch.<br/>\r\n";
  message += "It also reads a reed sensor to check if the garage door is currently open or closed.<br/>\r\n";
  message += "Furthermore, the ESP32 connects to a (currently) preconfigured WiFi where it sets up a simple webserver (this!) in order to make its values readble. Each variable is printed out indivudually as plain text under its own url and all together in a json response which makes it easy to use them in smart home apps (e.g. <a href=\"https://github.com/homebridge/\" target=\"_blank\">Homebridge</a>). There is also one option to manually overwrite the sensor readings by set car = true using a simple HTTP_GET request (see <a href=\"#control\">below</a> for Control). You can not configure the limit nor the WiFi otherwise than by recompiling and flashing the binary so far.</p>\r\n";
  message += "<hr>\r\n";
  message += "<h2>Status</h2>\r\n";
  message += "<ul><li><a href=\"/dist_cm\">dist_cm</a>: " + String(dist_cm) + "\r\n";
  message += "<li><a href=\"/car\">/car</a>: " + String(car) + "\r\n";
  message += "<li><a href=\"/garagedoor\">/garagedoor</a>: ";
  if (garagedoorOpened == false) {
    message += "(0) closed\r\n";
  } else if (garagedoorOpened == true) {
    message += "(1) open <small>(Hence we do not look for cars and disc_cm is -1)</small>\r\n";
  } else {
    message += "ERROR?!\r\n";
  }

  message += "<li>manual_car: (" + String(manual_car);
  if (manual_car==false) {
    message += ")\r\n";
  }
  else if (manual_car == true) {
    message += ") <small>(hence we do ignore <i>dist_cm</i> and do not look for cars, but still for <i>garagedoor</i>)</small>\r\n";
  }

  message += "<li><a href=\"/relay1state\">/relay1state</a>: ";
  if (relay1state == true) {
    message += "(1) closed\r\n";
  } else if (relay1state == false) {
    message += "(0) open\r\n";
  } else {
    message += "ERROR?!\r\n";
  }
  message += "<li><a href=\"/relay2state\">/relay2state</a>: ";
  if (relay2state == true) {
    message += "(1) closed\r\n";
  } else if (relay2state == false) {
    message += "(0) open\r\n";
  } else {
    message += "ERROR?!\r\n";
  }

  message += "<li><a href=\"/json\">/json</a> to get status report as a json response\r\n";
  message += "</ul><a name=\"control\"/><h2>Control</h2><ul>\r\n";
  message += "<li><a href=\"/manual_car\">/manual_car</a>: (" + String(manual_car) + ") <small>(to manually overwrite and pretend there is an object within the range limit</small>)\r\n";
  message += "<li><a href=\"/manual_reset\">/manual_reset</a> <small>(use this to get back to sensor readings)<br/>\r\n<small>(there is no option to manually set car to <em>false</em> for security reasons)</small>\r\n";
  message += "</ul>";
  message += "<p></p>\r\n";
  message += "<hr>\r\n";
  message += "<p><small>2023, by <a href=\"https://yves.app\" target=\"_blank\">Yves Jeanrenaud</a>. Licenced under GNU GPL-3. No warranty nor liability for anything. Please refer to <a href=\"https://github.com/yjeanrenaud/esp32_garagedoor_iot/\" target=\"_blank\">https://github.com/yjeanrenaud/esp32_garagedoor_iot/</a> for more details on this project.</small></p>\r\n";
  server.send(200, "text/html", message);
}
void handleDistcm() {
  String message = String(dist_cm) + "\r\n";
  server.send(200, "text/plain", message);
}

void handleCar() {
  String message = "";
  if (car == true) {
    message += "1\r\n";
  } else if (car == false) {
    message += "0\r\n";
  } else {
    message += "ERROR?!\r\n";
  }
  server.send(200, "text/plain", message);
}

void handleRelay1state() {
  String message = "";
  if (relay1state == true) {
    message += "1\r\n";
  } else if (relay1state == false) {
    message += "0\r\n";
  } else {
    message += "ERROR?!\r\n";
  }
  server.send(200, "text/plain", message);
}
void handleRelay2state() {
  String message = "";
  if (relay2state == true) {
    message += "1\r\n";
  } else if (relay2state == false) {
    message += "0\r\n";
  } else {
    message += "ERROR?!\r\n";
  }
  server.send(200, "text/plain", message);
}

void handleGaragedoor() {
  String message = "";
  if (garagedoorOpened == true) {
    message += "1\r\n";
  } else if (garagedoorOpened == false) {
    message += "0\r\n";
  } else {
    message += "ERROR?!\r\n";
  }
  server.send(200, "text/plain", message);
}
void handleJson() {
  String jsonString = "{\"dist_cm\": " + String(dist_cm) + ", \"car\": ";
  if (car == true) {
    jsonString += "1";
  } else if (car == false) {
    jsonString += "0";
  } else {
    jsonString += "ERROR?!";
  }
  jsonString += ", \"garagedoor\": ";
  if (garagedoorOpened == true) {
    jsonString += "1";
  } else if (garagedoorOpened == false) {
    jsonString += "0";
  } else {
    jsonString += "ERROR?!";
  }
  jsonString += ", \"manual_car\": ";
  if (manual_car == true) {
    jsonString += "1";
  } else if (manual_car == false) {
    jsonString += "0";
  } else {
    jsonString += "ERROR?!";
  }
  jsonString += ", \"relay1state\": ";
  if (relay1state == true) {
    jsonString += "1";
  } else if (relay1state == false) {
    jsonString += "0";
  } else {
    jsonString += "ERROR?!";
  }
  jsonString += ", \"relay2state\": ";
  if (relay2state == true) {
    jsonString += "1";
  } else if (relay2state == false) {
    jsonString += "0";
  } else {
    jsonString += "ERROR?!";
  }

  jsonString += "}";
  server.send(200, "application/json", jsonString);
}
void handleManualcar() {
  String message = "";
  if (manual_car == true) {
    message += "<h1>manual_car</h1><p>manual_car was <strong>on (true)</strong>.<br/>To reset and get back to actual sensor readings, use <a href=\"/manual_reset\">/manual_reset</a>.<br/>\r\n<small>Please note: There is no option to manually set car to <em>false</em> for security reasons.</small></p>\r\n<p><small><p><a href=\"/\">back...</a></small><p>\r\n";
  } else if (manual_car == false) {
    message += "<h1>manual_car</h1><p>manual_car was off (false) is now <strong>on (true)</strong>.<br/>To reset and get back to actual sensor readings, use <a href=\"/manual_reset\">manual_reset</a>.<br/>\r\n<small>Please Note: There is no option to manually set car to <em>false</em> for security reasons.</small></p>\r\n<p><small><a href=\"/\">back...</a></small><p>\r\n";
    manual_car = true;
  } else {
    message += "ERROR?!\r\n";
  }
  server.send(200, "text/html", message);
}
void handleManualreset() {
  String message = "";
  if (manual_car == true) {
    message += "<h1>manual_car</h1><p>manual_car was on (true)<br/> and is now reset to <strong>off (false)</strong></p>\r\n<p><small><a href=\"/\">back...</a></small><p>\r\n";
    manual_car = false;
    if (ledstate == true) {
      // we must reset the led, too. 
      ledstate = false;
      digitalWrite(led, ledstate);
    }

  } else if (manual_car == false) {
    message += "<h1>manual_car</h1><p>manual_car was already off (false)</p>\r\n<p><small><a href=\"/\">back...</a></small></p>\r\n";
  } else {
    message += "ERROR?!\r\n";
  }
  server.send(200, "text/html", message);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(button) == LOW) {
    if (buttonActive == false) {
      buttonActive = true;
      buttonTimer = millis();
    }
    if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) {
      Serial.println("long press detected");
      longPressActive = true;
      relay1state = !relay1state;
      digitalWrite(relay1, relay1state);
      relay2state = !relay2state;
      digitalWrite(relay2, relay2state);
      hardware_overwride = !hardware_overwride; //this is to enable the toggling of the relay1 even when there is a car
      manual_car = !manual_car; //toggle that, too
    }
  } else {
    if (buttonActive == true) {
      Serial.println("button press detected");
      if (longPressActive == true) {
        longPressActive = false;
      } else {
        //press action
        Serial.println("button press executed");
        ledstate = !ledstate;
        digitalWrite(led, ledstate);
        manual_car = ledstate; //toggle that, too
      }
      buttonActive = false;
    }
  }
  //first, check if the garage door is  closed (1).
  reedCurrentState = digitalRead(reedsensor);
  if (reedCurrentState == HIGH) { //cause homebridge expects open doors to be true and the sensor we used is NO
    garagedoorOpened = false;
  } else {  // if (reedCurrentState == LOW)
    garagedoorOpened = true;
  }

  if (hardware_overwride == true) { //kinda emergency mode
    //so we do not care about any readings, we want to switch the relays manually.
    Serial.println("hardware_overwride is active");
    car = false;
    dist_cm = -1;
    garagedoorOpened = true;
  }
  
  else { //work as inteded
    if (garagedoorOpened == false && hardware_overwride == false) {  // if garage door is closed (1) and the button was not pressed long, we will measure
      trigPulse();
      pulse = pulseIn(echo, HIGH, 200000);
      dist_cm = pulse / 58.82;
      // 340m/s
      // 34000cm/s

      /*
          100000 us - 17000 cm/s
              x us - 1cm
            1E6
        x = -----
            17E3
      */

      Serial.println(String(dist_cm) + " cm");
      Serial.println(String(pulse));
      
      if (manual_car == true) {
        //manual_car overwrites everything!. we do not care about dist_cm anymore
        car = true;
        relay1state = false;
        digitalWrite(relay1, relay1state);
        Serial.println("manual_car OVERWRITES, hence relay is open");
      //} else if (dist_cm <= limit && manual_car == false) {  // && reedsensor == 1) //object closer than limit; garage door status is irrelevant at this point
      } else if ((dist_cm <= (limit-diff) || dist_cm <= (limit+diff)) && manual_car == false) {  // && reedsensor == 1) //object closer than limit; garage door status is irrelevant at this point
        relay1state = false;
        car = true;
        digitalWrite(relay1, relay1state);
        Serial.println("relay is open");
      //} else if (dist_cm > limit || reedsensor == 0 && manual_car == false) {  //object is farer than limit OR garage door is open´
      } else if ((dist_cm > (limit-diff) || dist_cm > (limit+diff)) || reedsensor == 0 && manual_car == false) {  //object is farer than limit OR garage door is open
        car = false;
        relay1state = true;
        digitalWrite(relay1, relay1state);
        Serial.println("relay is closed");
      }
    }       //end of garagedoorOpened == true && hardware_overwride == true
    else {  //hence the door must be open (garagedoorOpened == false) OR the button was pressed long (hardware_overwride == true)
      car = false;
      dist_cm = -1;
      if (manual_car == true)  //we still want to know if the door should not move due to manual overwrite (manual_car == true)
      {
        //manual_car overwrites everything!. we do not care about dist_cm anymore, do NOT move the door
        car = true;
        relay1state = false;
        digitalWrite(relay1, relay1state);
        Serial.println("manual_car OVERWRITES, hence relay is open");
      }
      else {
        // so we can move the door, e.g. someone pressed the door button in the garage
        car = false;
        relay1state = true;
        digitalWrite(relay1, relay1state);
      }
    }
  }

  delay(200);
  server.handleClient();  //web server update
}
