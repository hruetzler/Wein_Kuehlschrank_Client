#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <LittleFS.h>
#include <ArduinoJson.h> 
#include <PubSubClient.h>

const char* mqtt_server = "mqtt_server";
const int mqtt_port = 8883;
bool zustandSchloss = true;

const int LED = D8;


const char* mqtt_user = "mqtt_user";
const char* mqtt_password = "mqtt_password";

JsonDocument daten;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void callback_mqtt(char* topic, byte* payload, unsigned int length);
void setup_wifi_mqtt();
void reconnect_mqtt();
void loadJson();
void saveJson();
void veraenderung();


void setup() {
  Serial.begin(115200);
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  setup_wifi_mqtt();
  loadJson();

  pinMode(LED, OUTPUT);


}

void loop() {
  if (!client.connected()) {
    Serial.println("Neu verbinden...");
    reconnect_mqtt();
  }
  client.loop();


}


void setup_wifi_mqtt(){
  File f = LittleFS.open("/wlanConfig.txt", "r");

   if (!f) {
    Serial.println("Datei konnte nicht geöffnet werden");
    return;
  }

  String ssid = f.readStringUntil('\n');
  String password = f.readStringUntil('\n');

  // mqtt_user = f.readStringUntil('\n');
  // mqtt_password = f.readStringUntil('\n');


  f.close();
  
  WiFi.begin(ssid, password);
  Serial.print("Connection to Wifi . . .");
  while (WiFi.status() != WL_CONNECTED )
  {
    delay(500);
    Serial.print(" .");
  }
  Serial.println("");

  Serial.println("Wifi verbunden");
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(4096);
  client.setCallback(callback_mqtt);

}

void callback_mqtt(char* topic, byte* payload, unsigned int length) {
  // Serial.print("Message arrived [");
  // Serial.print(topic);
  // Serial.print("] ");
  DeserializationError error = deserializeJson(daten, payload);
  if (error) {
    Serial.print(F("Parsing failed: "));
    Serial.println(error.c_str());
    return;
  }
  veraenderung();

  // Serial.println(daten.as<String>());

}

void reconnect_mqtt() {
  // Loop until we're reconnected
  while (!client.connected()) {
    espClient.setInsecure();
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("Connectet");
      // Once connected, publish an announcement...
      client.publish("henri/neuVerbunden", "hello world again");
      // ... and resubscribe
      client.subscribe("henri/inTopic");
      client.subscribe("henri/fridge");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loadJson(){
    File file = LittleFS.open("/json/daten.json", "r");
    DeserializationError error = deserializeJson(daten, file);

    if(error){
      Serial.println("Es ist ein Fehler beim lesen aufgetreten");

      daten["verriegelt"] = true;
      JsonDocument details;

      JsonArray zeiten = daten["zeiten"].to<JsonArray>();

      details["Name"] = "Test";
      details["Datum"] = "2023-09-26";
      details["Uhrzeit"] = "15:30";
      details["Dauer"] = 9339;
      details["Code"] = 893274;

      zeiten.add(details);
      veraenderung();
      
    }
    Serial.println(daten.as<String>());

    file.close();
    // zustandString = daten.as<String>();
    // Serial.println(zustand.as<String>());
  }

  void saveJson(){
    File file = LittleFS.open("/json/daten.json", "w");
    serializeJson(daten, file);
    file.close();
  }

  void veraenderung(){
    saveJson();
    if (daten["verriegelt"])
    {
      digitalWrite(LED, HIGH);
    }else
    {
      digitalWrite(LED, LOW);
    }

  }
