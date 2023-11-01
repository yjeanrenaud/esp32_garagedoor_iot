// Part of the ESP32 Garage door & car sensoric IoT project.
// This reads out the JSN SR04T ultrasonic sensor and prints its value to the serial console.
// It is usefull to define the limit for the main esp32_garagedoor_iot.ino.
// 2023, by Yves Jeanrenaud. Licenced under GNU GPL-3. No warranty nor liability for anything.
// Please refer to https://github.com/yjeanrenaud/esp32_garagedoor_iot/ for more details on this project.

//board specifics
#define button 0
#define relay1 16
#define relay2 17
#define led 23
//parking sensor hardware
#define trig 14  //Pin 33 trigger Tx
#define echo 13  //Pin 32 echo Rx

void trigPulse()
{
  digitalWrite(trig, HIGH);  //Trigger Pulse HIGH
  delayMicroseconds(10);     //for 10 micro seconds
  digitalWrite(trig, LOW);   //Trigger Pulse LOW
}

float pulse;     //echo time duration
float dist_cm;   //distance in cm

void setup() {
  // put your setup code here, to run once:
  //Relais: GPIO 16, GPIO 17; ac 250 V dc 30v
  //Taster GPIO 0 
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(button, INPUT);
  pinMode(led, OUTPUT);

  digitalWrite(button, HIGH); //pullup enabled
  digitalWrite(relay1, false);
  digitalWrite(relay2, false);
  digitalWrite(led, false);


  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  digitalWrite(trig, LOW);
  
  Serial.begin(115200);
  Serial.println("set up");
}

void loop() {
  // put your main code here, to run repeatedly:
    //Serial.println("looping");
  trigPulse();
  pulse = pulseIn(echo, HIGH, 200000);
  dist_cm = pulse/58.82;
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
  delay(200);
}
