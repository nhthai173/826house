//
// Created by Thái Nguyễn on 13/7/24.
//

/**
 * LED behavior:
 * - LED is ON when WiFi is disconnected
 * - LED is OFF when WiFi is connected
 * - LED is blinking when WiFi is connecting
 */

#ifndef NHA_SAU_CORE_WIFISTATUSLED_H
#define NHA_SAU_CORE_WIFISTATUSLED_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

void initWiFiStatusLED(uint8_t pin = LED_BUILTIN, bool active = LOW) {
    static uint8_t WIFI_STATUS_LED_PIN = pin;
    static bool WIFI_STATUS_LED_ACTIVE = active;
    static wl_status_t status = WL_IDLE_STATUS;
    static bool led_last_state = !WIFI_STATUS_LED_ACTIVE;
    static Ticker wifiStatusTicker;

    pinMode(WIFI_STATUS_LED_PIN, OUTPUT);
    wifiStatusTicker.attach_ms(500, []() {
        status = WiFi.status();
        if (status == WL_CONNECTED) {
            digitalWrite(WIFI_STATUS_LED_PIN, !WIFI_STATUS_LED_ACTIVE);
        } else if (status == WL_DISCONNECTED) {
            led_last_state = !led_last_state;
            digitalWrite(WIFI_STATUS_LED_PIN, led_last_state);
        } else {
            digitalWrite(WIFI_STATUS_LED_PIN, WIFI_STATUS_LED_ACTIVE);
        }
    });
};

#endif //NHA_SAU_CORE_WIFISTATUSLED_H
