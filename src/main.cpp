#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <LittleFS.h>
#include <ArduinoJson.h> 
#include <PubSubClient.h>
#include <Keypad.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
// #include <sys/time.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char* mqtt_server = "sruetzler.de";
const int mqtt_port = 8883;
bool zustandSchloss = true;

const int LED = D8;

struct tm date;

String code = "";
const byte COLS = 4; //4 Spalten
const byte ROWS = 4; //4 Zeilen

char hexaKeys[ROWS][COLS]={
{'1','2','3','A'},
{'4','5','6','B'},
{'7','8','9','C'},
{'*','0','#','D'}
};


byte colPins[COLS] = {D4,D5,D6,D7}; //Definition der Pins für die 3 Spalten
byte rowPins[ROWS] = {D0,D1,D2,D3}; //Definition der Pins für die 4 Zeilen
// char Taste;

Keypad Tastenfeld = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);



String mqtt_user;
String mqtt_password;

JsonDocument daten;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void checkCode();
void codeEingabe();
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

  timeClient.begin();
  timeClient.update();

  setTime(timeClient.getEpochTime());

}

void loop() {
  if (!client.connected()) {
    Serial.println("Neu verbinden...");
    reconnect_mqtt();
  }
  client.loop();
  codeEingabe();


}


void setup_wifi_mqtt(){
  File f = LittleFS.open("/wlanConfig.txt", "r");

   if (!f) {
    Serial.println("Datei konnte nicht geöffnet werden");
    return;
  }

  String ssid = f.readStringUntil('\n');
  String password = f.readStringUntil('\n');

  mqtt_user = f.readStringUntil('\n');
  mqtt_password = f.readStringUntil('\n');


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
  String clientId = "ESP8266Client";
  clientId += String(random(0xffff), HEX);

  while (!client.connected()) {
    espClient.setInsecure();
    if (client.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_password.c_str())) {
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

void codeEingabe(){
  char Taste = Tastenfeld.getKey();
  if (Taste)
  {
    Serial.println(Taste);
    if (Taste == 'A')
    {
      code = "";
      Serial.println("Code gelöscht");
      return;
    }
    if (Taste == 'D')
    {
      Serial.println("Code überprüfen");
      Serial.println(code);
      checkCode();
      code = "";
      return;
    }
    code += Taste;
    
  }
}



void checkCode(){
  int len = daten["zeiten"].size();

  for (int i = 0; i < len; i++)
  {
      String codeJ = daten["zeiten"][i]["Code"];
      if (code == codeJ){

        Serial.println("Der Code wurde richtig eingegeben");
        String datum = daten["zeiten"][i]["Datum"];
        String uhrzeit = daten["zeiten"][i]["Uhrzeit"];

        long long dateInMillis = daten["zeiten"][i]["DateInMillis"];
        int stundenInMillis = daten["zeiten"][i]["StundenInMillis"];

        Serial.println(dateInMillis);
        Serial.println(stundenInMillis);
        Serial.println(now());

        long long jetzt = now() * 1000;


        if (dateInMillis < jetzt && jetzt < dateInMillis + stundenInMillis)
        {
          Serial.println("Die Zeit stimmt überein");
          if (daten["verriegelt"]) {
            daten["verriegelt"] = false;
            veraenderung();
          }
        } else {
          Serial.println("Die Zeit stimmt nicht überein");
        }
        
        break;
      }
      
  }
}

 
