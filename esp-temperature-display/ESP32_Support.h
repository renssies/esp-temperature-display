#ifndef ESP32_Support_H
#define ESP32_Support_H

#if defined(ESP32)

#include <WiFi.h>    // Built in
#include <ESPmDNS.h> // Built in

// For ESP_DoubleResetDetector
#ifndef LED_BUILTIN
#define LED_BUILTIN       2         // Pin D2 mapped to pin GPIO2/ADC12 of ESP32, control on-board LED
#endif

#define LED_OFF     LOW
#define LED_ON      HIGH

#include <AsyncTCP.h>

String chipID = ESP.getChipModel();

#endif // ESP32
#endif // ESP32_Support_H