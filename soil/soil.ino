#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <string.h>

using namespace std;

// Define PIN
#define RELAY_PIN 15
#define SOIL_PIN 33
#define LDR_PIN 32

WiFiClient espClient;
PubSubClient client(espClient);

// Define Sending Struct ldr
typedef struct ldr_message{
  bool light;
  int board_id;
} ldr_message;

// Initial Struct
ldr_message ldrData;


// Define MAC_ADDRESS
uint8_t broadcastAddress[] = {0x24, 0x0A, 0xC4, 0x9F, 0x4F, 0xEC}; // 24:0A:C4:9F:4F:EC

// Define Function
void Connect_Wifi(); // For WiFi Connection
void Setup_MQTT(); // For Inital MQTT
void Setup_communication(); // Establish ESP_NOW Connection
void callback(char* topic, byte* payload, unsigned int length); // MQTT Subscribe Callback
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status); // Callback when sent Data

void soilMoistureFunction(void *param);
void ldrFunction(void *param);

// Task Handler
TaskHandle_t soilMoisture = NULL;
TaskHandle_t ldr = NULL;

// Global Variable
bool wateringStatus;
float moisture;
bool ldrStatus; // true = dark, false = day

// Config WiFi
char *ssid = "Peanut";
char *password = "0625901000";

// Config MQTT Server
#define mqtt_server "192.168.18.107"
#define mqtt_port 1883
// #define mqtt_server "0.0.0.0"
// #define mqtt_port 1883
#define mqtt_user ""
#define mqtt_password ""
#define mqtt_name "Peanut4"

// Other Define
#define BUADRATE 115200
#define BOARD_ID 4 // Change Board ID

void setup(){
  Serial.begin(BUADRATE);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println(digitalRead(RELAY_PIN)); 

  Connect_Wifi();
  Setup_MQTT();
  Setup_communication();

  // Create Task
  xTaskCreatePinnedToCore(soilMoistureFunction, "soilMoisture", 1024*10, NULL, 0, &soilMoisture, 0);
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
  if(client.connect(mqtt_name,mqtt_user,mqtt_password)){
        client.subscribe("embedded/watering/4"); // Change Board ID

    }else{
      Serial.println("failed");
    }
  client.setCallback(callback);  
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, payload);
  wateringStatus = doc["activate"];
  Serial.print("Activate : ");
  Serial.println(wateringStatus);
  digitalWrite(RELAY_PIN,wateringStatus);
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




// Implement Task Function
void soilMoistureFunction(void *param){
  while(1){
    digitalWrite(RELAY_PIN,wateringStatus);

    float soil_moisture = analogRead(SOIL_PIN);
    Serial.print("moisture : ");
    Serial.println(soil_moisture);
    /* ---------- Read Sensor Value ---------- */
    moisture = map(4095 - analogRead(SOIL_PIN), 0,4095,0,100);
    Serial.print("Moisture : ");
    Serial.println(moisture);
    /* --------------------------------------- */
    String json;
    DynamicJsonDocument doc(2048);
    doc["index"] = BOARD_ID;
    doc["moisture"] = moisture;

    serializeJson(doc,json);

    const char* json_char = json.c_str();
   
    client.publish("embedded/moisture", json_char);
    
    client.loop();
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }
}
