#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <analogWrite.h>
#include "Ultrasonic.h"


void wifiSetup();
void reconnect();
void callback(char*, byte*, unsigned int);

int mapHundredToEightBit(int percent);
int measureBrightness();
void changeBrightnessForPeriod(int brightness, int period);
void changeBrightness(int brightness);
void publishTrigger();
void publishBrightness(int brightness, int id);

const char* ssid = "iPhone floseph";
const char* password = "supersecret1177";

const char* mqqt_server = "broker.hivemq.com";

const int LED1 = 16;
Ultrasonic ultrasonic(18);
const int pressureSensorPin = 33;

const int triggerDistance = 5;
const int triggerWeight = 500;

const int lampID = 1;



WiFiClient espClient;
PubSubClient client(espClient);


void setup() {
  pinMode(LED1, OUTPUT);
  analogWrite(LED1, 8);

  Serial.begin(9600);
  wifiSetup();
  client.setServer(mqqt_server, 1883);
  client.setCallback(callback);
  reconnect();
}

void loop() {
  delay(100);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long rangeInCentimeters = ultrasonic.MeasureInCentimeters();
  if(rangeInCentimeters < triggerDistance){
    publishTrigger();
  }

  long value = analogRead(pressureSensorPin);
  if(value > triggerWeight){
    publishTrigger();
  }


}

void wifiSetup() {
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wifi..");
  }
  Serial.println("Connected to the Wifi network");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      //connected to Server
      Serial.println("connected");
      // subscripe to specific street channel. Street No. 1 in this example
      client.subscribe("/iot-thkoeln/smartlighting/1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();


  //controll flow for reacting on subscribed content.
  StaticJsonDocument<200> doc;
  deserializeJson(doc, messageTemp);
  const String type = doc["type"];


  if(String(topic) == "/iot-thkoeln/smartlighting/1") {
  
    //trigger
    if(type.equals("trigger") || type.equals("adjacentTrigger"))
    {
      changeBrightnessForPeriod(100, 10000);
    }
  }
}

int measureBrightness() {
  const int brightness = analogRead(LED1);
  return brightness;
}

int mapHundredToEightBit(int percent) {
  return map(percent, 0, 100, 0, 255);
}

void changeBrightnessForPeriod(int brightness, int period) {
  
  const int startingBrightness = measureBrightness();
  changeBrightness(brightness);
  publishBrightness();
  delay(period);
  changeBrightness(startingBrightness);
  publishBrightness();
}

void changeBrightness(int brightness){
  const int targetBrightnessInBit = mapHundredToEightBit(brightness);
  analogWrite(LED1, targetBrightnessInBit);
}

void publishTrigger() {
  const int capacity = JSON_OBJECT_SIZE(3);
  StaticJsonDocument<capacity> doc;
  doc["id"] = lampID;
  doc["type"] = "trigger";

  char output[128];
  serializeJson(doc, output);
  client.publish("/iot-thkoeln/smartlighting/1", output);
}

void publishBrightness() {
  const int capacity = JSON_OBJECT_SIZE(3);
  StaticJsonDocument<capacity> doc;

  doc["id"] = lampID;
  doc["brightness"] = map(measureBrightness(), 0, 255, 0, 100);
  doc["type"] = "update";

  char output[128];
  serializeJson(doc, output);
  client.publish("/iot-thkoeln/smartlighting/1", output);
}