//
// Created by Thái Nguyễn on 13/7/24.
//
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PCF8574.h>
#include <DoubleResetDetector.h>
#include <WiFiManager.h>

#include "helper.h"
#include "WiFiStatusLED.h"

#define I2C_ADDRESS 0x20                    // Default address
#define OUTPUT_ACTIVE LOW                   // output active level

PCF8574 pcf8574(I2C_ADDRESS);
DoubleResetDetector *drd;                   // Double click on reset button to enter configuration mode


bool isI2CCommunicationable(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

void setup() {
    Serial.begin(9600);
    while (!Serial)
        delay(50);

#if defined(DEBUG)
    delay(2000);    // Allow time to open a serial monitor
#endif

    // Start PCF8574
    pcf8574.begin();
    if (!isI2CCommunicationable(I2C_ADDRESS)) {
        D_Print("PCF8574 not found at %s. Try again after 5 seconds\n", String(I2C_ADDRESS).c_str());
        delay(5000);
        ESP.restart();
    }

    // Pin config
    pcf8574.pinMode(0, OUTPUT);
    pcf8574.pinMode(1, OUTPUT);
    pcf8574.pinMode(2, OUTPUT);
    pcf8574.pinMode(3, INPUT_PULLUP);
    pcf8574.pinMode(4, INPUT_PULLUP);
    pcf8574.pinMode(5, INPUT_PULLUP);
    pcf8574.pinMode(6, INPUT_PULLUP);

    pcf8574.digitalWrite(0, !OUTPUT_ACTIVE);
    pcf8574.digitalWrite(1, !OUTPUT_ACTIVE);
    pcf8574.digitalWrite(2, !OUTPUT_ACTIVE);

    // Init WiFi status LED
    initWiFiStatusLED();

    drd = new DoubleResetDetector(10, 0);
    if (drd->detectDoubleReset()) {
        D_Print("Start WiFi configuration\n");
        WiFiManager wm;
        wm.autoConnect();
    } else {
        // Connect to WiFi with saved credentials
        D_Print("Connect to WiFi \"%s\"\n", WiFi.SSID().c_str());
        WiFi.mode(WIFI_STA);
        WiFi.setAutoConnect(true);
        WiFi.persistent(true);
        WiFi.begin();
    }

    /* Reset saved WiFi credentials */
//    WiFi.disconnect(true, true);
//    ESP.eraseConfig();
//    delay(2000);


    D_Print("Setup done\n");
}

void loop() {
    drd->loop();    // double reset detector loop

    // Sample test
    pcf8574.digitalWrite(0, OUTPUT_ACTIVE);
    delay(2000);
    pcf8574.digitalWrite(1, OUTPUT_ACTIVE);
    delay(2000);
    pcf8574.digitalWrite(2, OUTPUT_ACTIVE);
    delay(2000);
    pcf8574.digitalWrite(0, !OUTPUT_ACTIVE);
    delay(2000);
    pcf8574.digitalWrite(1, !OUTPUT_ACTIVE);
    delay(2000);
    pcf8574.digitalWrite(2, !OUTPUT_ACTIVE);
}