#include <ESP8266WiFi.h> // Built in
#include <ESP8266mDNS.h> // Built in

#include <DoubleResetDetector.h> // From Library Manager, version 1.0.3
#include <Adafruit_NeoPixel.h> // From Library Manager, version 1.11.0
#include <WiFiManager.h> // From Library Manager, version 2.0.15-rc.1
#include <OneWire.h> // From Library Manager, version 2.3.7
#include <DallasTemperature.h> // From Library Manager, version 3.9.0
#include <PubSubClient.h> // https://pubsubclient.knolleary.net, version 2.8.0
#include <Preferences.h> //  From Library Manager, version 2.1.0

#define WEBSERVER_H // Important to add before importing ESPAsyncWebServer to fix linker issues with WiFiManager and ESPAsyncWebserver
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncTCP/tree/15476867dcbab906c0f1d47a7f63cdde223abeab
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer/tree/f71e3d427b5be9791a8a2c93cf8079792c3a9a26
#include <AsyncJson.h>
#include <ArduinoJson.h> // From Library Manager, version 6.21.2

#include "constants.h"

unsigned long digitOne = 0b00000000000000000000000011111111;

//                                     0                                   1                                      2                                3                                4                                      5                               6                                 7                                         8                              9                                         -
unsigned long digitMapping[11] = {0b00000000000011111111111111111111,0b00000000000000000000000011111111,0b00000000001111100111111110001111,0b000000000001111100100011111111111,0b00000000001100111100000011111111,0b00000000001111111100011111111001,0b00000000001111111111111111111001,0b00000000000011100000000011111111,0b00000000001111111111111111111111,0b00000000001111111100011111111111,0b00000000001100000000000000000000};

Adafruit_NeoPixel segment1Pixels(NUMBER_OF_PIXELS, SEGMENT1_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel segment2Pixels(NUMBER_OF_PIXELS, SEGMENT2_PIN, NEO_GRB + NEO_KHZ800);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature temperatureSensors(&oneWire);

char mqttServer[40];
char mqttPort[6] = "1883";
char mqttUsername[34];
char mqttPassword[34];

WiFiManager wifiManager;
WiFiManagerParameter paramMQTTServer("mqtt_server", "MQTT Server", mqttServer, 40);
WiFiManagerParameter paramMQTTPort("mqtt_port", "MQTT Port", mqttPort, 6);
WiFiManagerParameter paramMQTTUsername("mqtt_username", "MQTT Auth Username (leave blank if unauthenticated)", mqttUsername, 40, " type='username' autocapitalize='none'");
WiFiManagerParameter paramMQTTPassword("mqtt_password", "MQTT Auth Password (leave blank if unauthenticated)", mqttPassword, 40, " type='password'");

Preferences preferences;

AsyncWebServer webServer(80);

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

long lastPixelChangeTime = 0;
int previousChangedPixel = -1;

long lastTemperatureReadTime = 0;

long lastMQTTConnectTime = 0;

int brightness = 20; // 0-255

float temperature = -100;
int displayTemperature = -100;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

String hostname;
String mqttTemperatureTopic;
String mqttLastWillTopic;

// The order of pixels:
// D7               D6
//
//17	18	19	00    17	18	19	00
//16    			01    16    			01
//15	    		02    15	    		02
//14	21  20	03    14	21  20	03
//13	    		04    13	    		04
//12	    		05    12	    		05
//11          06    11          06
//10  09  08  07    10  09  08  07

//
// MARK: - Setup
//

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  hostname =             String(HOSTNAME_PREFIX) + "-" + ESP.getChipId();
  mqttTemperatureTopic = String(HOSTNAME_PREFIX) + "/" + ESP.getChipId() + "/temperature";
  mqttLastWillTopic =    String(HOSTNAME_PREFIX) + "/" + ESP.getChipId() + "/LWT";

  readPreferences();

  segment1Pixels.begin();
  segment2Pixels.begin();

  setupWiFiManager();

  setupHTTPServer();

  temperatureSensors.begin();

  uint8_t address;
  if (temperatureSensors.getAddress(&address, 0)) {
    Serial.print("Found probe at address: ");
    Serial.println(address);
  } else {
    Serial.print("No probe found, check pullup resistor.");
  }
}

void setupWiFiManager() {
  wifiManager.addParameter(&paramMQTTServer);
  wifiManager.addParameter(&paramMQTTPort);
  wifiManager.addParameter(&paramMQTTUsername);
  wifiManager.addParameter(&paramMQTTPassword);

  wifiManager.setConfigPortalBlocking(false); // Disable blocking so we can animate the display
  wifiManager.setConfigPortalTimeout(60);
  wifiManager.setHostname(hostname);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.setDebugOutput(false);

  if (drd.detectDoubleReset()) {
    showUnavailableLines(0, 0, brightness);

    Serial.println("Double reset detected");
    wifiManager.setConfigPortalTimeout(30);
    wifiManager.setConfigPortalTimeoutCallback(configModeTimeoutCallback);
    wifiManager.startConfigPortal(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);
    Serial.print("Config portal started, network password: ");
    Serial.println(CONFIG_AP_PASSWORD);
  } else if(wifiManager.autoConnect(CONFIG_AP_SSID, CONFIG_AP_PASSWORD)) {
    Serial.println("Connected...yaay :)");
    Serial.print("IP Address: ");
    Serial.print(WiFi.localIP());
    Serial.print(" MAC Address: ");
    Serial.println(WiFi.macAddress());
    if (!MDNS.begin(hostname)) {
      Serial.println("Failed to start mDNS");
    }
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "id", String(ESP.getChipId()));
    MDNS.addServiceTxt("http", "tcp", "ma", WiFi.macAddress().c_str());
    MDNS.addServiceTxt("http", "tcp", "hw", "esp8266");
  } else {
    showUnavailableLines(0, 0, brightness);

    Serial.print("Config portal started automatically, network password: ");
    Serial.println(CONFIG_AP_PASSWORD);
  }
}

void setupHTTPServer() {
  webServer.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404, "text/html", "<html><head><title>Page Not Found</title><body>Page Not Found</body></html>");
    }
  });

  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncJsonResponse * response = new AsyncJsonResponse();
    response->addHeader("X-Server","ESP Async Web Server");

    JsonObject root = response->getRoot();
    if (isValidTemperatureValue(temperature)) {
      root["temperature"] = round1(temperature);
    } else {
      root["temperature"] = nullptr;
    }
    if (isValidTemperatureValue(displayTemperature)) {
      root["display_temperature"] = displayTemperature;
    } else {
      root["display_temperature"] = nullptr;
    }
    JsonArray array = root.createNestedArray("endpoints");
    array.add("GET /heap");
    array.add("GET /info");
    array.add("POST /make-coffee");
    response->setLength();

    request->send(response);
  });

  webServer.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncJsonResponse *response = new AsyncJsonResponse();
    response->addHeader("X-Server","ESP Async Web Server");

    JsonObject root = response->getRoot();
    root["uptime"] = millis();
    root["heap"] = ESP.getFreeHeap();
    response->setLength();
    request->send(response);
  });

  webServer.on("/make-coffee", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send_P(418, "text/html", easter_egg);
  });

  webServer.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncJsonResponse *response = new AsyncJsonResponse();
    response->addHeader("X-Server","ESP Async Web Server");

    JsonObject root = response->getRoot();
    root["chip_id"] = ESP.getChipId();
    root["ip_address"] = WiFi.localIP();
    root["mac_address"] = WiFi.macAddress();
    root["uptime"] = millis();
    root["change_wifi_instructions"] = "Double press the reset button to enter config mode.";
    if (strlen(mqttServer) > 1) {
      root["mqtt_server"] = mqttServer;  
    } else {
      root["mqtt_server"] = nullptr;
    }
    JsonObject aboutObject = root.createNestedObject("about");
    aboutObject["original_by"] = "ghostleyjim";
    aboutObject["modified_by"] = "Sir-16 bit & renssies";
    aboutObject["hardware"] = "Arduino on a ESP8266 NodeMCU, 44 Neopixels and a DS18b20 temperature sensor";
    aboutObject["repository"] = "https://github.com/renssies/esp-temperature-display";
    response->setLength();
    
    request->send(response);
  });

  webServer.begin();
}

void readPreferences() {
  Serial.println("Reading preferenes");

  preferences.begin("esp-temp-display", true);

  preferences.getString(paramMQTTServer.getID(), mqttServer, 40);
  preferences.getString(paramMQTTPort.getID(), mqttPort, 6);
  preferences.getString(paramMQTTUsername.getID(), mqttUsername, 34);
  preferences.getString(paramMQTTPassword.getID(), mqttPassword, 34);

  if (atoi(mqttPort) <= 0) {
    strcpy(mqttPort, "1883");
  }

  paramMQTTServer.setValue(mqttServer, 40);
  paramMQTTPort.setValue(mqttPort, 6);
  paramMQTTUsername.setValue(mqttUsername, 34);
  paramMQTTPassword.setValue(mqttPassword, 34);

  preferences.end();

}

//
// MARK: - Loop
//

void loop() {
  connectMQTT();
  wifiManager.process();
  drd.loop();
  MDNS.update();

  readTemperature();

  if (wifiManager.getConfigPortalActive()) {
    updateNeopixelsConfigLoop();
  } else {
    updateNeopixelsTemperature();
  }
}

//
// MARK: - Neopixels
//

void updateNeopixelsTemperature() {
  // This code needs to be replaced to show the temperature.
  // Use `int displayTemperature` to get the temperature suitable for display.
  
  long now = millis();
  
  
  
  
  
  if ((now - lastPixelChangeTime) < PIXEL_CHANGE_INTERVAL) {
    return;
  }
  
  
  if (!isValidTemperatureValue(displayTemperature)) {
    clearPixels();
    showUnavailableLines(brightness, 0, 0);
    return;


  }


 int testTemp = displayTemperature;

//for(int testTemp = 0; testTemp <=30; testTemp++){


   clearPixels();
  for(int i = 0; i<=31; i++){
    if(bitRead(digitMapping[testTemp/10],i)){segment1Pixels.setPixelColor(i, segment1Pixels.Color(map(testTemp, 0, 30, 0,255), 0, map(testTemp, 0, 30, 255,0)));}

  }
   segment1Pixels.setBrightness(brightness);
  segment1Pixels.show();


  for(int i = 0; i<=31; i++){
    if(bitRead(digitMapping[testTemp%10],i)){segment2Pixels.setPixelColor(i, segment2Pixels.Color(map(testTemp, 0, 30, 0,255), 0, map(testTemp, 0, 30, 255,0)));}

  }
   segment2Pixels.setBrightness(brightness);
  segment2Pixels.show();
  //delay(250);
//}

  //Serial.println("");
  //segment1Pixels.setPixelColor(1, segment1Pixels.Color(255, 150, 0));


 // segment1Pixels.setBrightness(brightness);
  //segment1Pixels.show();

  /*
  segment2Pixels.setPixelColor(1, segment2Pixels.Color(255, 150, 200));
  segment2Pixels.setBrightness(brightness);
  segment2Pixels.show();
  /*
  int pixelToChange = previousChangedPixel + 1;

    if (pixelToChange >= NUMBER_OF_PIXELS) {
      clearPixels();
      previousChangedPixel = -1;
    } else {
      segment1Pixels.setPixelColor(pixelToChange, segment1Pixels.Color(255, 150, 0));
      segment1Pixels.setBrightness(brightness);
      segment1Pixels.show();
      segment2Pixels.setPixelColor(pixelToChange, segment2Pixels.Color(0, 150,70));
      segment2Pixels.setBrightness(brightness);
      segment2Pixels.show();
      previousChangedPixel = pixelToChange;
    }
    */
    lastPixelChangeTime = now;
}

void updateNeopixelsConfigLoop() {
  long now = millis();
  if ((now - lastPixelChangeTime) < PIXEL_CHANGE_INTERVAL) {
    return;
  }
  int pixelToChange = previousChangedPixel + 1;
  if (pixelToChange >= NUMBER_OF_PIXELS) {
    clearPixels();
    previousChangedPixel = -1;
  } else {
    segment1Pixels.setPixelColor(pixelToChange, segment1Pixels.Color(0, 0, 150));
    segment1Pixels.setBrightness(brightness);
    segment1Pixels.show();
    segment2Pixels.setPixelColor(pixelToChange, segment2Pixels.Color(0, 0, 150));
    segment2Pixels.setBrightness(brightness);
    segment2Pixels.show();
    previousChangedPixel = pixelToChange;
  }
  lastPixelChangeTime = now;
}

void clearPixels() {
  segment1Pixels.clear();
  segment2Pixels.clear();
}

void showUnavailableLines(uint8_t r, uint8_t g, uint8_t b) {
  segment1Pixels.setPixelColor(20, segment1Pixels.Color(r, g, b));
  segment1Pixels.setPixelColor(21, segment1Pixels.Color(r, g, b));
  segment2Pixels.setPixelColor(20, segment2Pixels.Color(r, g, b));
  segment2Pixels.setPixelColor(21, segment2Pixels.Color(r, g, b));

  segment1Pixels.show();
  segment2Pixels.show();
}

void hideUnavailableLines() {
  segment1Pixels.setPixelColor(20, 0);
  segment1Pixels.setPixelColor(21, 0);
  segment2Pixels.setPixelColor(20, 0);
  segment2Pixels.setPixelColor(21, 0);

  segment1Pixels.show();
  segment2Pixels.show();
}

//
// MARK: - Temperature
//

void readTemperature() {
  long now = millis();
  if ((now - lastTemperatureReadTime) > READ_TEMPERATURE_INTERVAL || lastTemperatureReadTime == 0) {
    if (!temperatureSensors.requestTemperaturesByIndex(0)) {
      temperature = -100;
      displayTemperature = -100;
      return;
    }
    float newTemperature = round1(temperatureSensors.getTempCByIndex(0));
    Serial.print("Temperature: ");
    Serial.println(newTemperature);
    temperature = newTemperature;
    displayTemperature = round(newTemperature);
    lastTemperatureReadTime = now;
    pulishTemperatureTopic(newTemperature);
  }
}

//
// MARK: - MQTT
//

void connectMQTT() {
  if (wifiManager.getConfigPortalActive()) {
    return;
  }
  if (mqttClient.connected()) {
    return;
  }
  String mqttServerStr = String(mqttServer);
  mqttServerStr.trim();
  if (mqttServerStr.isEmpty()) {
    return;
  }
  long now = millis();
  if ((now - lastMQTTConnectTime) < MQTT_CONNECT_INTERVAL && lastMQTTConnectTime != 0) {
    return;
  }
  int mqttPortInt = atoi(mqttPort);
  if (mqttPortInt <= 0) {
    mqttPortInt = 1883;
  }
  mqttClient.setServer(mqttServerStr.c_str(), mqttPortInt);
  if (mqttClient.connect(hostname.c_str(), mqttUsername, mqttPassword, mqttLastWillTopic.c_str(), 0, true, "offline")) {
    Serial.println("[MQTT]: Connected");
    publishHADiscoveryTopic();
    mqttClient.publish(mqttLastWillTopic.c_str(), "online", true);
  } else {
    Serial.println("[MQTT]: Failed to connect");
  }
  lastMQTTConnectTime = now;
  
}

void publishHADiscoveryTopic() {
  String topic = "homeassistant/sensor/" + hostname + "/config";

  DynamicJsonDocument payload(512);
  payload["dev_cla"] = "temperature";
  payload["stat_t"] = mqttTemperatureTopic;
  payload["uniq_id"] = hostname;
  payload["unit_of_meas"] = "°C";
  payload["name"] = "Temperature";
  payload["avty_t"] = mqttLastWillTopic;
  payload["exp_aft"] = 900; // Seconds after which the sensor is deemed unresponsive.

  JsonObject deviceObject = payload.createNestedObject("dev");
  deviceObject["name"] = "ESP Temperature Display";
  deviceObject["cu"] = String("http://") + WiFi.localIP().toString() + "/";
  deviceObject["mdl"] = "ESP8266";
  deviceObject["mf"] = "Ghostleyjim";

  JsonArray ids = deviceObject.createNestedArray("ids");
  ids.add(WiFi.macAddress());

  char payloadBuffer[512];
  serializeJson(payload, payloadBuffer);

  mqttClient.setBufferSize(512); // The discovery payload is rather big so we temporarily increase the max size.
  if(!mqttClient.publish(topic.c_str(), payloadBuffer, true)) {
    Serial.println("[MQTT]: Failed to publish HA discovery");
  } else {
    Serial.println("[MQTT]: Published HA discovery");
  }
  mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);
}

void pulishTemperatureTopic(float temperature) {
  if (!mqttClient.connected()) {
    return;
  }
  if (!isValidTemperatureValue(temperature)) {
    return;
  }
  char temperatureString[8];
  dtostrf(temperature, 1, 2, temperatureString);
  if (!mqttClient.publish(mqttTemperatureTopic.c_str(), temperatureString)) {
    Serial.println("[MQTT]: Failed to publish temperature");
  }
}

// 
// MARK: - Callbacks
// 

void saveConfigCallback() {
  Serial.println("Should save config");

  strcpy(mqttServer, paramMQTTServer.getValue());
  strcpy(mqttPort, paramMQTTPort.getValue());
  strcpy(mqttUsername, paramMQTTUsername.getValue());
  strcpy(mqttPassword, paramMQTTPassword.getValue());

  preferences.begin("esp-temp-display", false);
  
  preferences.putString(paramMQTTServer.getID(), mqttServer);
  preferences.putString(paramMQTTPort.getID(), mqttPort);
  preferences.putString(paramMQTTUsername.getID(), mqttUsername);
  preferences.putString(paramMQTTPassword.getID(), mqttPassword);

  preferences.end();

  mqttClient.disconnect();
  MDNS.end();

  wifiManager.reboot();
}

void configModeTimeoutCallback() {
  wifiManager.reboot();
}

//
// MARK: - Helpers 
//

double round1(double value) {
   return (int)(value * 10 + 0.5) / 10.0;
}

bool isValidTemperatureValue(float value) {
  return value > -30.0 && value < 120.0;
}

bool isValidTemperatureValue(int value) {
  return value > -30 && value < 120;
}
