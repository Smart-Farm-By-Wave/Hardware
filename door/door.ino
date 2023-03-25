// #include <Servo.h>
#include <ESP32Servo.h>
#include <HCSR04.h>

using namespace std;

// Define PIN
#define PIR_PIN 36
#define TRIG_PIN 22
#define ECHO_PIN 23
#define SERVO_PIN 32
#define LASER_PIN 15
#define LDR_PIN 39
#define ANGLE 60

/// Initial Instance
Servo myservo;
HCSR04 hc(TRIG_PIN, ECHO_PIN);

// Task Function
void doorFunction(void *param);
void doorSensorFunction(void *param);

// Task Handler
TaskHandle_t door = NULL;
TaskHandle_t doorSensor = NULL;

// Global Variable
bool isHumanMovement; // มีการขยับของคนหรือเปล่า
float handDistance; // ระยะทางจาก ของ มือ Threshold <= 2 cm
bool isHuman; //  ถ้า detect มือคนตรว sensor ได้เป็น true
bool isOpen; // สถานะประตู ปัจจุบัน
int countTime = 0;
bool detectLaser;

// Other Define
#define BUADRATE 115200


void setup() {
  // put your setup code here, to run once:
  pinMode(PIR_PIN, INPUT);
  myservo.setPeriodHertz(50); 
  myservo.attach(SERVO_PIN);
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH);
  myservo.write(0);

  Serial.begin(115200);  
}

void loop() {
    Serial.print("PIR : ");
    Serial.print(digitalRead(PIR_PIN));
    Serial.print(" Distance : ");
    Serial.println(hc.dist() <= 8);
    // Serial.print
    // if(digitalRead(PIR_PIN) && hc.dist() <= 8) {
    //   isHuman = true;
    // }else{
    //   isHuman = false;
    // }

    // Serial.print("isHuman : ");
    // Serial.println(isHuman);
    // Serial.print(analogRead(LDR_PIN));
    if(isOpen){ //ประตูเปิดอยู่
      if(analogRead(LDR_PIN) > 4000){
        isOpen = false;
        myservo.write(0);
      }
    }else{
      Serial.print("And: ");
      Serial.println(digitalRead(PIR_PIN) && hc.dist() <= 8.0);
      if(digitalRead(PIR_PIN) &&(hc.dist() <= 8)){
        // Serial.print("ANd : ");
        // Serial
        isOpen = true;
        myservo.write(ANGLE);
        delay(5000);
      } 
      ;
    } 
    Serial.print("Fuck you");
    delay(500);
}
