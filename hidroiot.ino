#include "DHT.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include <Wire.h> // I2C Arduino library
#include <BH1750FVI.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "SSID DA REDE";
const char* password = "SENHA DA REDE";
const char* mqtt_server = "SERVIDOR MQTT";
const int mqtt_port = 10362;
const char* mqtt_user = "USUARIO MQTT";
const char* mqtt_pass = "SENHA MQTT";
#define mqtt_topic "/greenhouse1/chanel1"
#define topic_room_temperature "/greenhouse1/room/temperature"
#define topic_room_humidity  "/greenhouse1/room/humidity"
#define topic_water_temperature "/greenhouse1/water/temperature"
#define topic_room_light "/greenhouse1/room/light"

#define DHTTYPE DHT11

const int pinDHT = 14;
const int pinDS18b20 = 13;
const int pinRelay = 2;


DHT dht(pinDHT, DHTTYPE);
OneWire oneWire(pinDS18b20);
DallasTemperature ds(&oneWire);
BH1750FVI LightSensor;
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int pinState = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == 'D') {
    digitalWrite(pinRelay, LOW);   // Turn the LED on (Note that LOW is the voltage level
    pinState = 0;
  } 
  else if ((char)payload[0] == 'L'){
    digitalWrite(pinRelay, HIGH);  // Turn the LED off by making the voltage HIGH
    pinState = 1;
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "HidroIOT-Estufa01";

    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("test", "Hello from HidroIOT");
      // ... and resubscribe
      client.subscribe(mqtt_topic);
      sendRelayState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, 0);
  dht.begin();
  ds.begin();
  LightSensor.begin();
  LightSensor.setMode(Continuously_High_Resolution_Mode);

  delay(50);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void getAndSendDHTData() {
  Serial.println();
  Serial.println("--- Get DHT11 Data");

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");
  
  
  
  client.publish(topic_room_temperature, String(t).c_str(), true);
  client.publish(topic_room_humidity, String(h).c_str(), true);
  Serial.println("Published Room Temperature and Humidity");
}

void getAndSendDS18b20() {
  Serial.println();
  Serial.println("--- Get DS18B20 Data");
  ds.requestTemperatures();
  float temp = ds.getTempCByIndex(0);
  Serial.print("Water Temperature: ");
  Serial.println(temp);
  client.publish(topic_water_temperature, String(temp).c_str(), true);
  Serial.println("Published Water Temperature");
  
}

void getAndSendLuxSensor() {
  Serial.println();
  Serial.println("--- Get LuxSensor Data");
  int lux;
  lux = LightSensor.getAmbientLight();
  Serial.print("Ambient light intensity: ");
  Serial.println(lux);
  client.publish(topic_room_light, String(lux).c_str(), true);
  Serial.println("Published light intensity");
  
}

void sendRelayState() {
  Serial.println();
  Serial.println("--- Get LuxSensor Data");
  Serial.print("Sended state of relay to broker!: ");
  if (pinState == 1) {
      client.publish(mqtt_topic, "L");
      Serial.println("D");
  }
 
  if (pinState == 0) {
      client.publish(mqtt_topic, "D");
      Serial.println("D");
  }
 
  
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    getAndSendDHTData();
    getAndSendDS18b20();
    getAndSendLuxSensor();
  }
}
