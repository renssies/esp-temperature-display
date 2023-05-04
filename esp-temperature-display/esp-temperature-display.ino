#include <ESP8266WiFi.h> // Built in
#include <ESP8266mDNS.h> // Built in

#include <DoubleResetDetector.h> // From Library Manager, version 1.0.3
#include <Adafruit_NeoPixel.h> // From Library Manager, version 1.11.0
#include <WiFiManager.h> // From Library Manager, version 2.0.15-rc.1
#include <OneWire.h> // From Library Manager, version 2.3.7
#include <DallasTemperature.h> // From Library Manager, version 3.9.0

#define WEBSERVER_H // Important to add before importing ESPAsyncWebServer to fix linker issues with WiFiManager and ESPAsyncWebserver
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncTCP/tree/15476867dcbab906c0f1d47a7f63cdde223abeab
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer/tree/f71e3d427b5be9791a8a2c93cf8079792c3a9a26
#include <AsyncJson.h>
#include <ArduinoJson.h> // From Library Manager, version 6.21.2

#include "constants.h"

Adafruit_NeoPixel segment1Pixels(NUMBER_OF_PIXELS, SEGMENT1_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel segment2Pixels(NUMBER_OF_PIXELS, SEGMENT2_PIN, NEO_GRB + NEO_KHZ800);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature temperatureSensors(&oneWire);

WiFiManager wifiManager;
AsyncWebServer webServer(80);

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

long lastPixelChangeTime = 0;
int previousChangedPixel = -1;

long lastTemperatureReadTime = 0;

int brightness = 20; // 0-255

float temperature = -100;
int displayTemperature = -100;

// The order of pixels:
// D6             D7
//
// 08 09 10 11    08 09 10 11
// 07       12    07       12
// 06       13    06       13
// 05       14    05       14
// 04 21 22 15    04 21 22 15
// 03       16    03       16
// 02       17    02       17
// 01 20 19 18    01 20 19 18

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    

  Serial.begin(115200);

  wifiManager.setConfigPortalBlocking(false); // Disable blocking so we can animate the display
  wifiManager.setConfigPortalTimeout(60);
  wifiManager.setHostname(HOSTNAME);

  wifiManager.setDebugOutput(true);

  if (drd.detectDoubleReset()) {
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

      if (!MDNS.begin(HOSTNAME)) {
        Serial.println("Failed to start mDNS");
      }
      MDNS.addService("http", "tcp", 80);
  } else {
    Serial.print("Config portal started automatically, network password: ");
    Serial.println(CONFIG_AP_PASSWORD);
  }

  segment1Pixels.begin();
  segment2Pixels.begin();

  setupHTTPServer();

  temperatureSensors.begin();
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
    if (temperature > -30 && temperature < 120) {
      root["temperature"] = round1(temperature);
    } else {
      root["temperature"] = nullptr;
    }
    if (displayTemperature > -30 && displayTemperature < 120) {
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
    AsyncJsonResponse * response = new AsyncJsonResponse();
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
    AsyncJsonResponse * response = new AsyncJsonResponse();
    response->addHeader("X-Server","ESP Async Web Server");

    JsonObject root = response->getRoot();
    root["ip_address"] = WiFi.localIP();
    root["mac_address"] = WiFi.macAddress();
    root["uptime"] = millis();
    root["change_wifi_instructions"] = "Double press the reset button to enter config mode.";
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

void loop() {
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

void readTemperature() {
  long now = millis();
  if ((now - lastTemperatureReadTime) > READ_TEMPERATURE_INTERVAL || lastTemperatureReadTime == 0) {
    Serial.println("Reading temperature");
    temperatureSensors.requestTemperaturesByIndex(0);
    float newTemperature = round1(temperatureSensors.getTempCByIndex(0));
    Serial.print("Temperature: ");
    Serial.println(newTemperature);
    temperature = newTemperature;
    displayTemperature = round(newTemperature);
    lastTemperatureReadTime = now;
  }
}

void updateNeopixelsTemperature() {
  // This code needs to be replaced to show the temperature.
  // Use `int displayTemperature` to get the temperature suitable for display.
  long now = millis();
  if ((now - lastPixelChangeTime) > PIXEL_CHANGE_INTERVAL) {
    Serial.println("Changing pixel");
    int pixelToChange = previousChangedPixel + 1;
    if (pixelToChange >= NUMBER_OF_PIXELS) {
      segment1Pixels.clear();
      segment2Pixels.clear();
      previousChangedPixel = -1;
    } else {
      segment1Pixels.setPixelColor(pixelToChange, segment1Pixels.Color(0, 150, 0));
      segment1Pixels.setBrightness(brightness);
      segment1Pixels.show();
      segment2Pixels.setPixelColor(pixelToChange, segment2Pixels.Color(0, 150, 0));
      segment2Pixels.setBrightness(brightness);
      segment2Pixels.show();
      previousChangedPixel = pixelToChange;
    }
    lastPixelChangeTime = now;
  }
}

void updateNeopixelsConfigLoop() {
  long now = millis();
  if ((now - lastPixelChangeTime) > PIXEL_CHANGE_INTERVAL) {
    Serial.println("Changing pixel (config loop)");
    int pixelToChange = previousChangedPixel + 1;
    if (pixelToChange >= NUMBER_OF_PIXELS) {
      segment1Pixels.clear();
      segment2Pixels.clear();
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
}

void configModeTimeoutCallback() {
  wifiManager.reboot();
}

double round1(double value) {
   return (int)(value * 10 + 0.5) / 10.0;
}
