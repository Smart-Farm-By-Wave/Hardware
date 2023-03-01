#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <string.h>

using namespace std;

// Define Cursor
#define DATA_CURSOR 13
#define UNIT_CURSOR 7

// Initial Instance

WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);   //Module IIC/I2C Interface บางรุ่นอาจจะใช้ 0x3f



// Define Sending Struct
// typedef struct data_messeage{
//   float temp;
//   float humidity;
// } data_messasge;

// Initial Struct
// data_messasge myData;

// Define Function
// void Setup_communication();
// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void Connect_Wifi(); // For WiFi Connection
void Setup_MQTT(); // For Inital MQTT
void callback(char* topic, byte* payload, unsigned int length); // MQTT Subscribe Callback
void setupLCD();
void printLCD();

// Task Function
void readAndPrintDataFunction(void *param);

// Task Handler
TaskHandle_t readAndPrintData = NULL;

// Global Variable
float temp = 0;
float humidity = 0;

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
  setupLCD();
  // Setup_communication();
  lcd.print(12);

  // Create Task
  xTaskCreatePinnedToCore(readAndPrintDataFunction, "readAndPrintData", 1024*10, NULL, 0, &readAndPrintData, 0);

}
void loop() {
  
}

// Implement Function

// void Setup_communication(){
//   WiFi.mode(WIFI_STA);
//   Serial.println(WiFi.macAddress());

//   // Init ESP-NOW
//   if (esp_now_init() != ESP_OK) {
//     Serial.println("Error initializing ESP-NOW");
//     return;
//   }
//   // เมื่อรับข้อมูลมา ให้ทำในฟังก์ชั่น OnDataRecv ที่เราสร้างไว้
//   esp_now_register_recv_cb(OnDataRecv);
// }

// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
//   memcpy(&myData, incomingData, sizeof(myData));
//   temp = myData.temp;
//   humidity = myData.humidity;
//   Serial.print("Temp: ");
//   Serial.print(temp);
//   Serial.print(" Humid: ");
//   Serial.println(humidity);
// }

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


void setupLCD(){
  lcd.begin();
  // lcd.noBacklight();   // ปิด backlight
  lcd.backlight();       // เปิด backlight 
}

void printLCD(){
  lcd.home();
  lcd.print("Temp: ");
  lcd.setCursor(7,0);
  lcd.print(temp);
  lcd.setCursor(13,0);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Humid: ");
  lcd.setCursor(7,1);
  lcd.print(humidity);
  lcd.setCursor(13,1);
  lcd.print("%");
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, payload);
  temp = doc["temp"];
  humidity = doc["humidity"];
  Serial.print("Temp : ");
  Serial.println(temp);
  Serial.print(" Humidity : ");
  Serial.println(humidity);
  printLCD();
  
}

// Implement Task Function


void readAndPrintDataFunction(void *param){
  while(1){
    
    if(client.connect(mqtt_name,mqtt_user,mqtt_password)){
      client.subscribe("embedded/plantStatus");
      // Serial.println("Hello World");
    }else{
      Serial.println("failed");
    }
    client.loop();
    // vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}
