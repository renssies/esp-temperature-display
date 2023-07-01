#ifndef PB_CONTANTS
#define PB_CONTANTS 1

// The pin for the first segment number.
#define SEGMENT1_PIN 1//D6

// The pin for the second segment number.
#define SEGMENT2_PIN 2//D7

// The number of Neopixels on one segment.
#define NUMBER_OF_PIXELS 22

// The pin used for one wire communication.
#define ONE_WIRE_PIN 3//D2

// Number of seconds after reset during which a subseqent reset will be detected as a double reset.
#define DRD_TIMEOUT 10 // seconds

// The RTC Memory Address for the DoubleResetDetector to use.
#define DRD_ADDRESS 0

// The minimum interval between temperature reads.
#define READ_TEMPERATURE_INTERVAL 3000 // miliseconds

// The minumum interval between pixel changes when running a loop (for in config mode)
#define PIXEL_CHANGE_INTERVAL 500 // miliseconds

// The minimum interval between MQTT connection attempts
#define MQTT_CONNECT_INTERVAL 10000 // miliseconds

// The network SSID for the config portal.
#define CONFIG_AP_SSID "temp-display-cfg"

// The network password for the config portal.
#define CONFIG_AP_PASSWORD "1337hacker"

// The hostname for WiFi and mDNS.
#define HOSTNAME_PREFIX "esp-temp-display"

const char easter_egg[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>I'm a teapot</title>
  </head>
  <body>
  
  <h1>I'm a teapot!</h1>
  <pre id="teapot">
|-------------------|
|              ;,'  |
|      _o_    ;:;'  |
|  ,-.'---`.__ ;    |
| ((j`=====',-'     |
|  `-\     /        |
|     `-=-'     hjw |
|-------------------|
</pre>
  </body>
</html>
)=====";

#endif 