//
// Created by Thái Nguyễn on 13/7/24.
//
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PCF8574.h>
#include <DoubleResetDetector.h>
#include <WiFiManager.h>

#include "WiFiStatusLED.h"
#include "GenericOutput.h"
#include "GenericInput.h"
#include "secret.h"
#include "FirebaseIoT.h"

#define I2C_ADDRESS 0x20                    // Default address
#define OUTPUT_ACTIVE LOW                   // output active level

PCF8574 pcf8574(I2C_ADDRESS);
DoubleResetDetector *drd;                    // Double click on reset button to enter configuration mode
GenericOutput R1(pcf8574, 0, OUTPUT_ACTIVE);
GenericOutput R2(pcf8574, 1, OUTPUT_ACTIVE);
GenericOutput R3(pcf8574, 2, OUTPUT_ACTIVE);
GenericInput S1(13, INPUT_PULLUP, LOW, 50, true);


void streamCallback(FirebaseStream data);

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

    S1.onActive([]() {
        R1.toggle();
    });

    fb_syncState(&R1, "/R1");
    fb_syncState(&R2, "/R2");
    fb_syncState(&R3, "/R3");

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
    WiFi.begin("C21.20", "diamondc2120");
    }

    /* Reset saved WiFi credentials */
//    WiFi.disconnect(true, true);
//    ESP.eraseConfig();
//    delay(2000);


    D_Print("Setup done\n");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        D_Print(".");
    }
    D_Print("\n");


    // Firebase setup
    fb_setup(USER_EMAIL,
             USER_PASSWORD,
             API_KEY,
             DATABASE_URL);
    fb_setStreamCallback(streamCallback);
}

void loop() {
    drd->loop();    // double reset detector loop
    fb_loop();      // Firebase loop
}


void streamCallback(FirebaseStream data) {
    Serial.printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                  data.streamPath().c_str(),
                  data.dataPath().c_str(),
                  data.dataType().c_str(),
                  data.eventType().c_str());

    if (data.dataPath() == "/") {
        FirebaseJsonData jdata;
        data.jsonObjectPtr()->get(jdata, "R1");
        if (jdata.success) {
            R1.setState(jdata.boolValue);
        }
        data.jsonObjectPtr()->get(jdata, "R2");
        if (jdata.success) {
            R2.setState(jdata.boolValue);
        }
        data.jsonObjectPtr()->get(jdata, "R3");
        if (jdata.success) {
            R3.setState(jdata.boolValue);
        }
    } else if (data.dataPath() == "/R1") {
        R1.setState(data.boolData());
    } else if (data.dataPath() == "/R2") {
        R2.setState(data.boolData());
    } else if (data.dataPath() == "/R3") {
        R3.setState(data.boolData());
    }

    printResult(data); // see addons/RTDBHelper.h
    Serial.println();

    // This is the size of stream payload received (current and max value)
    // Max payload size is the payload size under the stream path since the stream connected
    // and read once and will not update until stream reconnection takes place.
    // This max value will be zero as no payload received in case of ESP8266 which
    // BearSSL reserved Rx buffer size is less than the actual stream payload.
    Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());
}