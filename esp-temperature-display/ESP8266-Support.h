#ifndef ESP8266_Support_H
#define ESP8266_Support_H 

#if defined(ESP8266)

#include <ESP8266WiFi.h>  // Built in
#include <ESP8266mDNS.h>  // Built in

// ESP_DoubleResetDetector Settings
#define LED_ON      LOW
#define LED_OFF     HIGH
#define ESP8266_DRD_USE_RTC false
#define ESP_DRD_USE_LITTLEFS true

#include <ESPAsyncTCP.h>  // https://github.com/me-no-dev/ESPAsyncTCP/tree/15476867dcbab906c0f1d47a7f63cdde223abeab

String chipID = ESP.getChipId();

#endif // ESP8266
#endif // ESP8266_Support_H