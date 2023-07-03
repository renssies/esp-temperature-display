#include "constants.h"

class State {
 public:
  long lastPixelChangeTime = 0;
  int previousChangedPixel = -1;
  long lastTemperatureReadTime = 0;
  long lastMQTTConnectTime = 0;
  int brightness = 10;  // 0-255
  float temperature = -100;
  int displayTemperature = -100;

  bool shouldUpdatePixels() {
    // return (millis() - lastPixelChangeTime) > PIXEL_CHANGE_INTERVAL;
    return !((millis() - lastPixelChangeTime) < PIXEL_CHANGE_INTERVAL); 
  }

  bool shouldReadTemprature() {
    return (millis() - lastTemperatureReadTime) > READ_TEMPERATURE_INTERVAL || lastTemperatureReadTime == 0;
  }

  bool shouldConnectMQTT() {
    // return ((millis() - lastMQTTConnectTime) > MQTT_CONNECT_INTERVAL || lastMQTTConnectTime == 0);
    return !((millis() - lastMQTTConnectTime) < MQTT_CONNECT_INTERVAL && lastMQTTConnectTime != 0);
  }

  void updateLastPixelChangeTime() {
    lastPixelChangeTime = millis();
  }

  void updateLastTemperatureReadTime() {
    lastTemperatureReadTime = millis();
  }

  void updateLastMQTTConnectTime() {
    lastMQTTConnectTime = millis();
  }
};