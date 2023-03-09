// #include <Servo.h>
#include <ESP32Servo.h>
#include <HCSR04.h>

using namespace std;

// Define PIN
#define PIR_PIN 2
#define TRIG_PIN 22
#define ECHO_PIN 23
#define SERVO_PIN 32
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

// Other Define
#define BUADRATE 115200


void setup() {
  // put your setup code here, to run once:
  myservo.setPeriodHertz(50); 
  myservo.attach(SERVO_PIN);

  Serial.begin(115200);  

  xTaskCreatePinnedToCore(doorFunction, "door", 1024*10, NULL, 0, &door, 0);
  xTaskCreatePinnedToCore(doorSensorFunction, "doorSensor", 1024*10, NULL, 2, &doorSensor, 1);
}

void loop() {

}

// Implement Task Function

void doorFunction(void *param){
  while(1){
    if(isHumanMovement && handDistance <= 2.0) isHuman = true;
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

void doorSensorFunction(void *param){
  while(1){
    if(isHuman != isOpen){
      if(isHuman){
        myservo.write(ANGLE);
        vTaskDelay(5000/portTICK_PERIOD_MS);
      }else{
        myservo.write(0);
      }
    }
  }
}