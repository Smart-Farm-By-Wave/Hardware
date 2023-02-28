#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <string.h>

#include <MQ2.h>
#include <DHT.h>

using namespace std;

// Define PIN
#define MQ2_PIN 35
#define DHT_PIN 13
#define FLAME_PIN 34 
#define RAIN_PIN 39
#define WATER_PIN 36

// Initial Instance
#define DHTTYPE DHT11

WiFiClient espClient;
PubSubClient client(espClient);

MQ2 mq2(MQ2_PIN);
DHT dht(DHT_PIN, DHTTYPE);


// Define Sending Struct
typedef struct data_messeage{
  float temp;
  float humidity;
} data_messasge;

// Initial Struct
data_messasge myData;

// Define MAC_ADDRESS
uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0x50, 0xB1, 0x30};

// Define Function
void Connect_Wifi(); // For WiFi Connection
void Setup_MQTT(); // For Inital MQTT
void Setup_communication(); // Establish ESP_NOW Connection
void callback(char* topic, byte* payload, unsigned int length); // MQTT Subscribe Callback

// Task Function
void smokeFireWaterFunction(void *param);
void tempHumidRainFunction(void *param);
void sendDataToBoardFunction(void *param);

// Task Handler
TaskHandle_t smokeFireWater = NULL;
TaskHandle_t tempHumidRain = NULL;
TaskHandle_t sendDataToBoard = NULL;

// Global Variable
bool smoke;
bool fire;
float waterLevel;
float rainAmount;
float temp;
float humidity;

// Config WiFi
char *ssid = "Peanut";
char *password = "0625901000";

// Config MQTT Server
#define mqtt_server "broker.emqx.io"
#define mqtt_port 1883
#define mqtt_user ""
#define mqtt_password ""
#define mqtt_name "Peanut"

// Other Define
#define BUADRATE 115200



void setup(){
  Serial.begin(BUADRATE);

  Connect_Wifi();
  Setup_MQTT();

  // Sensor Init
  mq2.begin();
  dht.begin();  

  // Create Task
  xTaskCreatePinnedToCore(smokeFireWaterFunction, "smokeFireWater", 1024*10, NULL, 0, &smokeFireWater, 0);
  xTaskCreatePinnedToCore(tempHumidRainFunction, "tempHumidRain", 1024*10, NULL,0, &tempHumidRain, 0);
}

void loop(){

}


// Implement Function

void Connect_Wifi(){
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print("OK! IP=");
    Serial.println(WiFi.localIP());
}

void Setup_MQTT(){
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);  
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
}

// Implement Task Function

void smokeFireWaterFunction(void *param){
  while(1){
    /* ---------- Read Sensor Value ---------- */
    // smoke
    float co = mq2.readCO();
    float smoke_analog = mq2.readSmoke();
    smoke = false;
    // Fire
    float flame = analogRead(FLAME_PIN);
    fire = false;
    // Water
    waterLevel = map(4096 - analogRead(WATER_PIN),1750,2500,0,100);
    Serial.println(4096 - analogRead(WATER_PIN));
    if(waterLevel <= 0) waterLevel = 0;
    if(waterLevel >= 100) waterLevel = 100;
    /* --------------------------------------- */
    String json;
    // char* json = new char[1024];
    DynamicJsonDocument doc(1024);
    doc["smoke"] = smoke;
    doc["fire"] = fire;
    doc["waterLevel"] = waterLevel;

    serializeJson(doc, json);

    // strcpy(json, temp_json);

    const char *json_char = json.c_str();
    if(client.connect(mqtt_name,mqtt_user,mqtt_password)){
      client.publish("embedded/central", json_char);
      ;
    }else{
      Serial.println("failed");
     }
    client.loop();
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void tempHumidRainFunction(void *param){
  while(1){
    /* ---------- Read Sensor Value ---------- */
    // temp
    temp = dht.readTemperature();
    // humid
    humidity = dht.readHumidity();
    // rain
    rainAmount = map(4096 - analogRead(RAIN_PIN), 1750, 2500, 0, 50);
    if(waterLevel <= 0) waterLevel = 0;
    if(waterLevel >= 50) waterLevel = 50;
    /* --------------------------------------- */
    // JSON Init
    String json;
    // char* json = new char[1024];
    DynamicJsonDocument doc(1024);
    doc["temp"] = temp;
    doc["humidity"] = humidity;
    doc["rainAmount"] = rainAmount; 

    serializeJson(doc, json);
    // strcpy(json, temp_json);
    const char *json_char = json.c_str();
    if(client.connect(mqtt_name,mqtt_user,mqtt_password)){
      client.publish("embedded/plantStatus", json_char);
    }else{
      Serial.println("failed");
     }
    client.loop();
    vTaskDelay(5000/portTICK_PERIOD_MS);
  }

}






// #include "wifi.h"
// #include <MQ2.h>
// #include <DHT.h>

// #include <esp_now.h>
// #include <WiFi.h>
// using namespace std;
// // Define PIN

// #define MQ2_PIN 35
// #define DHT_PIN 13
// #define FLAME_PIN 39 
// #define RAIN_PIN 34
// #define WATER_PIN 36

// #define DHTTYPE DHT11
// //Initial Instance
// MQ2 mq2(MQ2_PIN);
// DHT dht(DHT_PIN, DHTTYPE);

// //Define Struct
// typedef struct data_messeage{
//   float temp;
//   float humidity;
// } data_messasge;
 
// data_messasge myData;

// //Define MAC_ADDRESS
// uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0x50, 0xB1, 0x30};//ส่งไปหาเฉพาะ mac address


// void setup_communication();


// void setup() {
//   // put your setup code here, to run once:
//   Serial.begin(115200);
//   setup_communication();
  
//   Connect_Wifi();

//   //Sensor Init
//   mq2.begin();
//   dht.begin();

// }

// void loop() {
//   // put your main code here, to run repeatedly:
//   // float lpg = mq2.readLPG();
//   // float co = mq2.readCO();
//   // float smoke = mq2.readSmoke();
//   // Serial.print("LPG: ");
//   // Serial.println(lpg);
//   // Serial.print("CO: ");
//   // Serial.println(co);
//   // Serial.print("Smoke: ");
//   // Serial.println(smoke);

//   float h = dht.readHumidity();
//   // Read temperature as Celsius (the default)
//   float t = dht.readTemperature();
//   // Read temperature as Fahrenheit (isFahrenheit = true)
//   float f = dht.readTemperature(true);

//   // Check if any reads failed and exit early (to try again).
//   if (isnan(h) || isnan(t) || isnan(f)) {
//     Serial.println(F("Failed to read from DHT sensor!"));
//     return;
//   }

//   // Compute heat index in Fahrenheit (the default)
//   float hif = dht.computeHeatIndex(f, h);
//   // Compute heat index in Celsius (isFahreheit = false)
//   float hic = dht.computeHeatIndex(t, h, false);

//   Serial.print(F("Humidity: "));
//   Serial.print(h);
//   Serial.print(F("%  Temperature: "));
//   Serial.print(t);
//   Serial.print(F("°C "));
//   // Serial.print(f);
//   // Serial.print(F("°F  Heat index: "));
//   // Serial.print(hic);
//   // Serial.print(F("°C 
//   // Serial.print(hif);
//   // Serial.println(F("°F"));

//   // int flame = analogRead(FLAME_PIN);
//   // Serial.println(flame);

//   // float rain_level = analogRead(RAIN_PIN);

//   // Serial.println(rain_level); //1400 MAX //LOW 3200
//   myData.temp = t;
//   myData.humidity = h;
//   esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
//   if (result == ESP_OK) {
//     Serial.println("Sent with success");
//   }
//   else {
//     Serial.println("Error sending the data");
//   }
//   delay(2000);

//   delay(1000);
// }

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("\r\nLast Packet Send Status:\t");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }

// void setup_communication(){
//   WiFi.mode(WIFI_STA);

//   // สั่งให้เริ่ม ESP-NOW
//   if (esp_now_init() != ESP_OK) {
//     Serial.println("Error initializing ESP-NOW");
//     return;
//   }

//   //เมื่อส่งให้ทำฟังก์ชั่น OnDataSend ที่เราสร้างขึ้น
//   esp_now_register_send_cb(OnDataSent);

//   // Register peer
//   esp_now_peer_info_t peerInfo{};
//   memcpy(peerInfo.peer_addr, broadcastAddress, 6);
//   peerInfo.channel = 0;
//   peerInfo.encrypt = false;

//   // เชื่อมต่ออุปกรณ์ที่ต้องการสื่อสาร
//   if (esp_now_add_peer(&peerInfo) != ESP_OK) {
//     Serial.println("Failed to add peer");
//     return;
//   }
// }

// #include <esp_now.h>
// #include <WiFi.h>

// // แก้ไขค่า mac ตัวที่ต้องการส่งไปหา
// uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0x50, 0xB1, 0x30};//ส่งไปหาเฉพาะ mac address
// //uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};//ส่งไปหาทุกตัว

// typedef struct struct_message { // สร้างตัวแปรแพ็จเกจแบบ struct
//   char a[32];
//   int b;
//   float c;
//   bool d;
// } struct_message;

// struct_message myData; // ตัวแปรแพ็คเกจที่ต้องการส่ง

// //เมื่อส่งข้อมูลมาทำฟังก์ชั่นนี้
// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("\r\nLast Packet Send Status:\t");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }

// void setup() {
//   Serial.begin(115200);
//   //ตั้งเป็นโหมด Wi-Fi Station
//   WiFi.mode(WIFI_STA);

//   // สั่งให้เริ่ม ESP-NOW
//   if (esp_now_init() != ESP_OK) {
//     Serial.println("Error initializing ESP-NOW");
//     return;
//   }

//   //เมื่อส่งให้ทำฟังก์ชั่น OnDataSend ที่เราสร้างขึ้น
//   esp_now_register_send_cb(OnDataSent);

//   // Register peer
//   esp_now_peer_info_t peerInfo = {};
//   memcpy(peerInfo.peer_addr, broadcastAddress, 6);
//   peerInfo.channel = 0;
//   peerInfo.encrypt = false;

//   // เชื่อมต่ออุปกรณ์ที่ต้องการสื่อสาร
//   if (esp_now_add_peer(&peerInfo) != ESP_OK) {
//     Serial.println("Failed to add peer");
//     return;
//   }
// }

// void loop() {
//   // สร้างตัวแปรแพ็คเกจที่ต้องการส่ง
//   strcpy(myData.a, "THIS IS A CHAR");
//   myData.b = random(1, 20);
//   myData.c = 1.2;
//   myData.d = false;

//   // สั่งให้ส่งข้อมูล
//   esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

//   if (result == ESP_OK) {
//     Serial.println("Sent with success");
//   }
//   else {
//     Serial.println("Error sending the data");
//   }
//   delay(2000);
// }

