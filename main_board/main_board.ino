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
// typedef struct data_messeage{
//   float temp;
//   float humidity;
// } data_messasge;

// // Initial Struct
// data_messasge myData;

// Define MAC_ADDRESS
// uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0x50, 0xB1, 0x30};

// Define Function
void Connect_Wifi(); // For WiFi Connection
void Setup_MQTT(); // For Inital MQTT
// void Setup_communication(); // Establish ESP_NOW Connection
void callback(char* topic, byte* payload, unsigned int length); // MQTT Subscribe Callback
// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status); // Callback when sent Data


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
  // Setup_communication();

  // Sensor Init
  mq2.begin();
  dht.begin();  

  // Create Task
  xTaskCreatePinnedToCore(smokeFireWaterFunction, "smokeFireWater", 1024*10, NULL, 0, &smokeFireWater, 0);
  xTaskCreatePinnedToCore(tempHumidRainFunction, "tempHumidRain", 1024*10, NULL,2, &tempHumidRain, 1);
  // xTaskCreatePinnedToCore(sendDataToBoardFunction, "sendDataToBoard", 1024*10, NULL,0, &sendDataToBoard, 0);
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

// void Setup_communication(){
//    WiFi.mode(WIFI_STA);
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

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


// Implement Task Function

void smokeFireWaterFunction(void *param){
  while(1){
    Serial.println("Smoke");
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
    // Serial.println(4096 - analogRead(WATER_PIN));
    if(waterLevel <= 0) waterLevel = 0;
    if(waterLevel >= 100) waterLevel = 100;
    /* --------------------------------------- */
    String json;
    // char* json = new char[1024];
    DynamicJsonDocument doc(2048);
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
    Serial.println("Temp");
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
    DynamicJsonDocument doc(2048);
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

// void sendDataToBoardFunction(void *param){
//   while(1){
//     myData.temp = temp;
//     myData.humidity = humidity;
//     esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
//     if (result == ESP_OK) {
//       Serial.println("Sent with success");
//     }
//     else {
//       Serial.println("Error sending the data");
//     }
//     vTaskDelay(1000/portTICK_PERIOD_MS);
//   }
// }



