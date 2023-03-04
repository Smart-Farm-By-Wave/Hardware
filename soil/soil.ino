#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <string.h>

using namespace std;

// Define PIN
#define RELAY_PIN 15
#define SOIL_PIN 33

WiFiClient espClient;
PubSubClient client(espClient);

// Define Function
void Connect_Wifi(); // For WiFi Connection
void Setup_MQTT(); // For Inital MQTT
void callback(char* topic, byte* payload, unsigned int length); // MQTT Subscribe Callback

// Global Variable
bool wateringStatus;
float moisture;

// Config WiFi
char *ssid = "Peanut";
char *password = "password";

// Config MQTT Server
#define mqtt_server "broker.emqx.io"
#define mqtt_port 1883
#define mqtt_user ""
#define mqtt_password ""
#define mqtt_name "Peanut"

// Other Define
#define BUADRATE 115200
#define BOARD_ID 1

void setup(){
  Serial.begin(BUADRATE);

  Connect_Wifi();
  Setup_MQTT();

}

void loop(){
  /* ---------- Read Sensor Value ---------- */
  moisture = map(analogRead(SOIL_PIN), 0,1023,0,100);
  /* --------------------------------------- */
  String json;
  DynamicJsonDocument doc(2048);
  doc["index"] = 1;
  doc["moisture"] = moisture;

  serializeJson(doc,json);

  const char* json_char = json.c_str();
  if(client.connect(mqtt_name,mqtt_user,mqtt_password)){
      client.subscribe("embedded/test/watering/",);
      client.publish("embedded/test/moisture", json_char);
      ;
    }else{
      Serial.println("failed");
     }
    client.loop();
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
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, payload);
  float activate = doc["activate"];
  Serial.print("Activate : ");
  Serial.println(activate);
  digitalWrite(RELAY_PIN,activate);
}
