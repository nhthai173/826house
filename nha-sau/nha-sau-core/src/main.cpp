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
#include "GenericOutput.h"

#define I2C_ADDRESS 0x20                    // Default address
#define OUTPUT_ACTIVE LOW                   // output active level

PCF8574 pcf8574(I2C_ADDRESS);
DoubleResetDetector *drd;                    // Double click on reset button to enter configuration mode
GenericOutput R1(pcf8574, 0, OUTPUT_ACTIVE, 2000);
GenericOutput R2(pcf8574, 1, OUTPUT_ACTIVE, 2000);
GenericOutput R3(pcf8574, 2, OUTPUT_ACTIVE, 2000);


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

    // Init WiFi status LED
    initWiFiStatusLED();

    // Setup to turn on R1 -> R2 -> R3 -> R1 -> ...
    R1.onPowerOff([]() {
        R2.on();
    });
    R2.onPowerOff([]() {
        R3.on();
    });
    R3.onPowerOff([]() {
        R1.on();
    });

    R1.onPowerChanged([](){
        D_Print("R1: %s\n", R1.getStateString().c_str());
    });
    R2.onPowerChanged([](){
        D_Print("R2: %s\n", R2.getStateString().c_str());
    });
    R3.onPowerChanged([](){
        D_Print("R3: %s\n", R3.getStateString().c_str());
    });

    // Start PCF8574
    pcf8574.begin();
    if (!isI2CCommunicationable(I2C_ADDRESS)) {
        D_Print("PCF8574 not found at %s. Try again after 5 seconds\n", String(I2C_ADDRESS).c_str());
        delay(5000);
        ESP.restart();
    }

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

    // Start the first relay
    R1.on();
}

void loop() {
    drd->loop();    // double reset detector loop
}