//
// Created by Thái Nguyễn on 13/7/24.
//

#ifndef NHA_SAU_CORE_HELPER_H
#define NHA_SAU_CORE_HELPER_H

#define DEBUG   // Comment this line to disable debug print

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

#if defined(DEBUG)
#define D_Print(...) Serial.printf(__VA_ARGS__)
#else
#define D_Print(...)
#endif

#endif //NHA_SAU_CORE_HELPER_H
