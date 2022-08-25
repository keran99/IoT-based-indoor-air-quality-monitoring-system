/**************************************************************************************
 * This code can be loaded on the ESP32. It is able to have the following data:       *
 * temperature, humidity, gas concentration (AQI), rssi [+ GPS and ID] and send       *
 * them via MQTT or CoAP protocol to the client.                                      *
 * Also, through the MQTT and HTTP protocol it is able to send such data to influxDB. *
 **************************************************************************************/
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define LED_BUILTIN 2
#define BUTTON_PIN 5

int led_state = LOW;
int button_state;
int last_button_state;
int num_gas = 0;
bool check5GasValue = false;
float gas0=0;
float gas1=0;
float gas2=0;
float gas3=0;
float gas4=0;

char tempConv[8];
char humiConv[8];
char gasConv[8];
char rsiiConv[8];
char AQIConv[8];

float MAX_GAS_VALUE = 1000;
float MIN_GAS_VALUE = 100;

#define DHT_SENSOR_PIN  19
#define DHT_SENSOR_TYPE DHT22
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

int Gas_analog = 34;
#define OLED_ADDR 0x3C 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);
int SAMPLE_FREQUENCY = 5;
WiFiClient espClient;
PubSubClient client(espClient);

const char* serverName = "http://xxx.xxx.x.xxx:xxxx/getValues";

const char* ssid = "";
const char* password = "";

const char* gps = "44.497, 11.356";
const char* ID = "ESP32Client-01";

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("Connected, your IP is: ");
  Serial.println((WiFi.localIP())); 
}

void connectmqttserver() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32-01"), "", "") {
      Serial.print("connected");
      client.subscribe("MAX_GAS_VALUE");
      client.subscribe("MIN_GAS_VALUE");
      client.subscribe("SAMPLE_FREQUENCY");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(".Message ");
  String messageTemp;

  for(int i=0; i<length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if(String(topic) == "MAX_GAS_VALUE") {
    MAX_GAS_VALUE = messageTemp.toFloat();
  }
  if(String(topic) == "MIN_GAS_VALUE") {
    MIN_GAS_VALUE = messageTemp.toFloat();
  } 
  if(String(topic) == "SAMPLE_FREQUENCY") {
    SAMPLE_FREQUENCY = messageTemp.toFloat();
  }
}


// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port);

// CoAP server endpoint url callback
void callback_temperature(CoapPacket &packet, IPAddress ip, int port);
void callback_humidity(CoapPacket &packet, IPAddress ip, int port);
void callback_gas(CoapPacket &packet, IPAddress ip, int port);
void callback_maxGasValue(CoapPacket &packet, IPAddress ip, int port);
void callback_minGasValue(CoapPacket &packet, IPAddress ip, int port);
void callback_sampleFrequency(CoapPacket &packet, IPAddress ip, int port);
void callback_allData(CoapPacket &packet, IPAddress ip, int port);

// UDP and CoAP class
WiFiUDP udp;
Coap coap(udp);

// CoAP server endpoint URL
void callback_temperature(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Temperature]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  coap.sendResponse(ip, port, packet.messageid, tempConv);
  Serial.println(tempConv);
}

void callback_humidity(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Humidity]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  coap.sendResponse(ip, port, packet.messageid, humiConv);
}

void callback_gas(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Gas]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  coap.sendResponse(ip, port, packet.messageid, gasConv);
}

void callback_rssi(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[RSSI]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  coap.sendResponse(ip, port, packet.messageid, rsiiConv);
}

void callback_aqi(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[AQI]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  coap.sendResponse(ip, port, packet.messageid, AQIConv);
}

void callback_gps(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[GPS]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  coap.sendResponse(ip, port, packet.messageid, gps);
}

void callback_maxGasValue(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Max Gas Value]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  String message(p);
  MAX_GAS_VALUE = message.toFloat();
  if (MAX_GAS_VALUE == message.toFloat()) {
    coap.sendResponse(ip, port, packet.messageid, "OK");
  } else {
    coap.sendResponse(ip, port, packet.messageid, "ERROR");
  }
}

void callback_minGasValue(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Min Gas Value]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  String message(p);
  Serial.println(message);
  MIN_GAS_VALUE = message.toFloat();
  Serial.print("MIN: ");
  Serial.println(MIN_GAS_VALUE);
  if (MIN_GAS_VALUE == message.toFloat()) {
    coap.sendResponse(ip, port, packet.messageid, "OK");
  } else {
    coap.sendResponse(ip, port, packet.messageid, "ERROR");
  }
}

void callback_sampleFrequency(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Sample Frequency]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  String message(p);
  Serial.println(message);
  SAMPLE_FREQUENCY = message.toFloat();
  Serial.print("SAMPLE_FREQUENCY: ");
  Serial.println(SAMPLE_FREQUENCY);
  if (SAMPLE_FREQUENCY == message.toFloat()) {
    coap.sendResponse(ip, port, packet.messageid, "OK");
  } else {
    coap.sendResponse(ip, port, packet.messageid, "ERROR");
  }
}

void callback_allData(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[All data]");
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  char allDatas[200];
  strcpy(allDatas," Temperature: ");
  strcat(allDatas,tempConv);
  strcat(allDatas,";\n Humidity: ");
  strcat(allDatas,humiConv);
  strcat(allDatas,";\n Gas: ");
  strcat(allDatas,gasConv);
  strcat(allDatas,";\n RSSI: ");
  strcat(allDatas,rsiiConv);
  strcat(allDatas,";\n AQI: ");
  strcat(allDatas,AQIConv);
  strcat(allDatas,";\n GPS: ");
  strcat(allDatas,gps);
  strcat(allDatas,";\n ID: ");
  strcat(allDatas,ID);
  
  coap.sendResponse(ip, port, packet.messageid, allDatas);
}

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Coap Response got]");
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  Serial.println(p);
}


void setup() {
  Serial.begin(9600);
  dht_sensor.begin();
  display.begin( SSD1306_SWITCHCAPVCC, OLED_ADDR);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  button_state = digitalRead(BUTTON_PIN);
  
  initWiFi();
  client.setServer("xxx.xxx.x.xxx", xxxx);
  client.setCallback(callback);
  connectmqttserver();

  // CoAP setup
  Serial.println("Setup Callback Temperature");
  coap.server(callback_temperature, "temperature");
  Serial.println("Setup Callback Humidity");
  coap.server(callback_humidity, "humidity");
  Serial.println("Setup Callback Gas");
  coap.server(callback_gas, "gas");
  Serial.println("Setup Callback RSSI");
  coap.server(callback_rssi, "rssi");
  Serial.println("Setup Callback AQI");
  coap.server(callback_aqi, "aqi");
  Serial.println("Setup Callback GPS");
  coap.server(callback_gps, "gps");
  Serial.println("Setup Callback Max gas value");
  coap.server(callback_maxGasValue, "set/maxGasValue");
  Serial.println("Setup Callback Min gas value");
  coap.server(callback_minGasValue, "set/minGasValue");
  Serial.println("Setup Callback Sample frequency");
  coap.server(callback_sampleFrequency, "set/sampleFrequency");
  Serial.println("Setup Callback All data");
  coap.server(callback_allData, "allData");
  // client response callback.
  // this endpoint is single callback.
  Serial.println("Setup Response Callback");
  coap.response(callback_response);

  // start coap server/client
  coap.start();
}


void loop() {
  // clean dislay
  display.clearDisplay();
  display.display();
  display.setTextSize(1.9);
  display.setTextColor(WHITE);
  display.setCursor(0, 5); 
  
  // get values from sensors
  float humi  = dht_sensor.readHumidity();
  float tempC = dht_sensor.readTemperature();
  float tempF = dht_sensor.readTemperature(true);
  int gassensorAnalog = analogRead(Gas_analog);
  // check whether the reading is successful or not
  if ( isnan(tempC) || isnan(tempF) || isnan(humi)) {
    display.println("Errore: sensore DHT22");
    display.display();
  } else if (isnan(gassensorAnalog)){
    display.println("Errore: sensore MQ-2");
    display.display();
  } else {
    // Calculate AQI
    if (num_gas == 0){
      gas0 = gassensorAnalog;
    } else if (num_gas == 1){
      gas1 = gassensorAnalog;
    } else if (num_gas == 2){
      gas2 = gassensorAnalog;
    } else if (num_gas == 3){
      gas3 = gassensorAnalog;
    } else if (num_gas == 4){
      gas4 = gassensorAnalog;
    }

    float avgGas;
    if (check5GasValue == false){
      if (num_gas > 3) {
        check5GasValue = true;
      }
      avgGas = (gas0 + gas1 + gas2 + gas3 + gas4)/ (num_gas+1);
    } else {
      avgGas = (gas0 + gas1 + gas2 + gas3 + gas4)/5;
    }
    
    int AQI;
    if (avgGas >= MAX_GAS_VALUE){
      AQI = 0;
    } else if ((avgGas>= MIN_GAS_VALUE) && (avgGas < MAX_GAS_VALUE)){
      AQI = 1;
    } else {
      AQI = 2;
    }
    
    num_gas = num_gas+1;
    if (num_gas == 5) {
      num_gas = 0;
    }
    
    display.print("T: ");
    display.println(tempC);
    display.print("H: ");
    display.print(humi);
    display.println("%");
    display.print("G: ");
    display.print(gassensorAnalog);
    display.print("  (AQI: ");
    display.print(AQI);
    display.println(")");
    display.print("RSSI: ");
    display.println(WiFi.RSSI());
    display.print("GPS: ");
    display.println(gps);
    display.print("ID: ");
    display.println(ID);
    display.display();

    last_button_state = button_state;
    button_state = digitalRead(BUTTON_PIN);

    // Change protocol
    if (last_button_state == HIGH && button_state == LOW) {
      Serial.println("Change protocol");
      led_state = !led_state;
      digitalWrite(LED_BUILTIN, led_state);
      delay(1000);
    }

    // Json with all data
    StaticJsonDocument<200> doc;
    doc["TEMPERATURE"] = tempC;
    doc["HUMIDITY"] = humi;
    doc["GAS"] = gassensorAnalog;
    doc["GPS"] = gps;
    doc["ID"] = ID;
    doc["AQI"] = AQI;
    doc["RSSI"] = WiFi.RSSI();
    char telemetryAsJson[256];
    serializeJson(doc, telemetryAsJson);

    dtostrf(tempC,2,2,tempConv);
    dtostrf(humi,2,2,humiConv);
    dtostrf(gassensorAnalog,4,0,gasConv);
    dtostrf(WiFi.RSSI(),3,0,rsiiConv);
    dtostrf(AQI,1,0,AQIConv);

    if (!led_state){
      display.println("P: MQTT");
      display.display();
      if(!client.connected()){
        connectmqttserver();
      }
      client.loop();
      //Serial.println("Invio pacchetto");
      client.publish("esp32",telemetryAsJson);
    } else {
      display.println("P: COAP/HTTP");
      display.display();
      coap.loop();

      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.PUT(telemetryAsJson);
      if(httpResponseCode>0){
        String response = http.getString();   
        Serial.println(httpResponseCode);
        Serial.println(response);          
      }else{
        Serial.print("Error on sending PUT Request: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    }
  }
  delay (SAMPLE_FREQUENCY*1000);
}
