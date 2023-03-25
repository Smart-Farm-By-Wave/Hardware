#include <WiFi.h>
#include <esp_wifi.h>
#include <PubSubClient.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <string.h>

#include <MQ2.h>
#include <DHT.h>
#include <HCSR04.h>


using namespace std;

// Define PIN
#define MQ2_PIN 35
#define DHT_PIN 13
#define FLAME_PIN 34 
#define RAIN_PIN 39
#define WATER_PIN 36
// Define PIN for Ultrasonic
#define TRIG_PIN 2
#define ECHO_PIN_WATER 16
#define ECHO_PIN_RAIN 17


// Initial Instance
#define DHTTYPE DHT11

WiFiClient espClient;
PubSubClient client(espClient);

MQ2 mq2(MQ2_PIN);
DHT dht(DHT_PIN, DHTTYPE);
HCSR04 hc(TRIG_PIN, new int[2]{ECHO_PIN_WATER, ECHO_PIN_RAIN,}, 2); //initialisation class HCSR04 (trig pin , echo pin)


// Define Sending Struct
typedef struct data_messeage{
  float temp;
  float humidity;
} data_messasge;

// Define Sending Struct ldr
typedef struct ldr_message{
  bool light;
  int board_id;
} ldr_message;

// // Initial Struct
data_messasge myData;
ldr_message ldrData;

// Define MAC_ADDRESS
uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0x50, 0xB1, 0x30};

// Define Function
void Connect_Wifi(); // For WiFi Connection
void Setup_MQTT(); // For Inital MQTT
void Setup_communication(); // Establish ESP_NOW Connection
void callback(char* topic, byte* payload, unsigned int length); // MQTT Subscribe Callback
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status); // Callback when sent Data
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);


// Task Function
void smokeFireWaterFunction(void *param);
void tempHumidRainFunction(void *param);
void sendDataToBoardFunction(void *param);
void timerFunction(void *param);

// Task Handle
TaskHandle_t smokeFireWater = NULL;
TaskHandle_t tempHumidRain = NULL;
TaskHandle_t sendDataToBoard = NULL;
TaskHandle_t timer = NULL;

// Global Variable
bool smoke;
bool fire;
float waterLevel;
float rainAmount;
float temp;
float humidity;
bool light;
bool ldrStatus[] = {false,false,false,false}; // true = dark, false = day

// Config WiFi
char *ssid = "Peanut";
char *password = "0625901000";

// Config MQTT Server
#define mqtt_server "192.168.18.107"
#define mqtt_port 1883
#define mqtt_user ""
#define mqtt_password ""
#define mqtt_name "Peanut0"

// Other Define
#define BUADRATE 115200



void setup(){
  Serial.begin(BUADRATE);

  Connect_Wifi();
  Setup_MQTT();
  Setup_communication();

  // Sensor Init
  mq2.begin();
  dht.begin();  

  // Create Task
  xTaskCreatePinnedToCore(smokeFireWaterFunction, "smokeFireWater", 1024*10, NULL, 0, &smokeFireWater, 0);
  xTaskCreatePinnedToCore(tempHumidRainFunction, "tempHumidRain", 1024*10, NULL,2, &tempHumidRain, 1);
  xTaskCreatePinnedToCore(sendDataToBoardFunction, "sendDataToBoard", 1024, NULL,0, &sendDataToBoard, 0);
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

void Setup_communication(){
   WiFi.mode(WIFI_AP_STA);
  // สั่งให้เริ่ม ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  //เมื่อส่งให้ทำฟังก์ชั่น OnDataSend ที่เราสร้างขึ้น
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // เชื่อมต่ออุปกรณ์ที่ต้องการสื่อสาร
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&ldrData, incomingData, sizeof(ldrData));
  light = ldrData.light;
  // Serial.print("Light: ");
  // Serial.println(light);
}


// Implement Task Function
int countTime = 0;
void smokeFireWaterFunction(void *param){
  while(1){
    /* ---------- Read Sensor Value ---------- */
    // smoke
    float co = mq2.readCO();
    float smoke_analog = mq2.readSmoke();
    // Fire
    bool flame = (analogRead(FLAME_PIN) ? false: true);
    if(analogRead(FLAME_PIN) <= 500) {
      fire = true;
    }else{
      fire = false;
    }
    
    Serial.print("Flame: ");
    Serial.println(flame);
    Serial.println(analogRead(FLAME_PIN));
    // Water

    // waterLevel = hc.dist(i);
    Serial.print("Distance Water : ");
    Serial.println(hc.dist(1)); // 2.04 Max , Min 10.46
    
    waterLevel = map(hc.dist(1),2,10.91, 0, 100 );
    waterLevel = 100 - waterLevel;
    if(waterLevel > 100) waterLevel = 100;
    if(waterLevel < 0) waterLevel = 0;
    // waterLevel = map(4096 - analogRead(WATER_PIN),1750,2500,0,100);
    // Serial.println(4096 - analogRead(WATER_PIN));
    // if(waterLevel <= 0) waterLevel = 0;
    // if(waterLevel >= 100) waterLevel = 100;
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
    countTime += 1;
    if(countTime >= 10){
      // fire = !fire;
      // smoke = !smoke;
      countTime = 0;
    }
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
    rainAmount = map(18.19 - hc.dist(0),1,19, 0, 17 );
    if(rainAmount <= 0) waterLevel = 0;
    if(rainAmount >= 17) waterLevel = 17;
    Serial.print("Distance Rain : ");
    Serial.println(hc.dist(0)); // 2.04 Max , Min
    /* --------------------------------------- */
    // JSON Init
    String json;
    // char* json = new char[1024];
    DynamicJsonDocument doc(2048);
    doc["temp"] = temp;
    doc["humidity"] = humidity;
    doc["rainAmount"] = waterLevel; 

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

void sendDataToBoardFunction(void *param){
  while(1){
    myData.temp = temp;
    myData.humidity = humidity;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    vTaskDelay(5000/portTICK_PERIOD_MS);
  }
}



