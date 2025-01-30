#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SimpleTimer.h>
#include <FirebaseESP8266.h>
#include <ESP8266WebServer.h>
#include <ElegantOTA.h>

#include "GenericInput.h"
#include "GenericOutput.h"
#include "DelayTask.h"

#include "pinconfig.h"
#include "nhtCredential.h"

#define FIRMWARE_VERSION 3

uint32_t AUTO_OFF = 20 * 60 * 1000;

GenericInput c1Btn(C1_BTN_PIN, INPUT_PULLUP, TRIGGER_STATE);
GenericInput c2Btn(C2_BTN_PIN, INPUT_PULLUP, TRIGGER_STATE);
GenericInput c3Btn(C3_BTN_PIN, INPUT_PULLUP, TRIGGER_STATE);
GenericOutput c1(C1_PIN, C1_ACTIVE_STATE, AUTO_OFF);
GenericOutput c2(C2_PIN, C2_ACTIVE_STATE);
GenericOutput c3(C3_PIN, C3_ACTIVE_STATE);

FirebaseData fbdo;
FirebaseData stream;
SimpleTimer timer;
ESP8266WebServer server(80);
nhtCredential wCre;

void afterOnline();

void firstRun();

bool str2bool(String str);

/**
 * @brief Set channel state from JSON object
 * @param json
 */
void setRelayFromJSON(FirebaseJson json);

/**
 * @brief Set relay state
 * @param ch c1 | c2 | c3 | all
 * @param v bool
 * @return
 */
bool setRelay(String ch, bool v);

void syncFirebase();

void streamCb(StreamData stream);

void streamTimeoutCallback(bool timeout);

DelayTask syncFBTask(2000, syncFirebase);

void setup() {
    Serial.begin(9600);
    while (!Serial) delay(20);

    c1Btn.onActive([]() { c1.toggle(); });
    c2Btn.onActive([]() { c2.toggle(); });
    c3Btn.onActive([]() { c3.toggle(); });
    c1.onPowerChanged([](){ syncFBTask.tick(); });
    c2.onPowerChanged([](){ syncFBTask.tick(); });
    c3.onPowerChanged([](){ syncFBTask.tick(); });
    c1.onAutoOff([](){
        c2.setState(false);
        c3.setState(false);
    });

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    ElegantOTA.begin(&server);

    server.on("/firmware", HTTP_ANY, [&]() { server.send(200, "text/plain", String(FIRMWARE_VERSION)); });

    server.on("/restart", HTTP_ANY, [&]() {
        server.send(200, "text/plain", "restart success");
        timer.setTimeout(2000, []() { ESP.restart(); });
    });

    server.on("/set", HTTP_ANY, [&]() {
        String ch = "";
        String v = "";
        if (server.hasArg("ch")) {
            ch = server.arg("ch");
        }
        if (server.hasArg("v")) {
            v = server.arg("v");
        }
        if (ch != "" && v != "") {
            if (setRelay(ch, str2bool(v))) {
                server.send(200, "text/plain", "success");
                return;
            }
        }
        server.send(400, "text/plain", "fail");
    });

    String AP_SSID = "Den Phong Tho";
    String AP_PASSWORD = "";

    wCre.connectedCallback(afterOnline);
    wCre.begin(&server, &AP_SSID, &AP_PASSWORD);

}

void afterOnline() {
    Serial.println("Connecting Firebase");

    Firebase.begin(wCre.getDBURL().c_str(), wCre.getDBKEY().c_str());
    Firebase.reconnectWiFi(true);
    Firebase.setDoubleDigits(5);

#if defined(ESP8266)
    stream.setBSSLBufferSize(2048, 512);
#endif

    if (!Firebase.beginStream(stream, "/den_phong_tho/all"))
        Serial.printf("stream begin error, %s\n\n", stream.errorReason().c_str());
    Firebase.setStreamCallback(stream, streamCb, streamTimeoutCallback);

    timer.setTimeout(1000L, firstRun);
}

void firstRun() {
    if (Firebase.ready()) {
        FirebaseJson json;
        json.set("localIP", WiFi.localIP().toString());
        json.set("lastOnline/.sv", "timestamp");
        Firebase.updateNodeSilentAsync(fbdo, "/den_phong_tho", json);

        json.clear();
        Firebase.getJSON(fbdo, "/den_phong_tho/all", &json);
        setRelayFromJSON(json);
    } else {
        timer.setTimeout(1000L, firstRun);
    }
}

bool str2bool(String str) {
    str.toLowerCase();
    if (str == "true" || str == "on" || str == "high" || str == "1")
        return true;
        // else if(str == "false" || str == "off" || str == "low" || str == "0" || str == "")
    else
        return false;
}

bool setRelay(String ch, bool v) {
    if (ch.startsWith("/")) {
        ch = ch.substring(1);
    }
    if (ch == "c1") {
        c1.setState(v);
        return true;
    } else if (ch == "c2") {
        c2.setState(v);
        return true;
    } else if (ch == "c3") {
        c3.setState(v);
        return true;
    } else if (ch == "all") {
        c1.setState(v);
        c2.setState(v);
        c3.setState(v);
        return true;
    }
    return false;
}

void setRelayFromJSON(FirebaseJson json) {
    FirebaseJsonData d;

    json.get(d, "c1");
    if (d.success) {
        c1.setState(str2bool(d.stringValue));
    }
    json.get(d, "c2");
    if (d.success) {
        c2.setState(str2bool(d.stringValue));
    }
    json.get(d, "c3");
    if (d.success) {
        c3.setState(str2bool(d.stringValue));
    }
}

void syncFirebase() {
    if (Firebase.ready()) {
        FirebaseJson json;
        json.set("c1", c1.getState());
        json.set("c2", c2.getState());
        json.set("c3", c3.getState());
        Firebase.updateNodeSilentAsync(fbdo, "/den_phong_tho/all", json);
    }
}

void streamCb(StreamData data) {
    Serial.printf("stream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                  data.streamPath().c_str(),
                  data.dataPath().c_str(),
                  data.dataType().c_str(),
                  data.eventType().c_str());
    Serial.println();
    Serial.printf("Received stream string:  %s\n", data.stringData().c_str());
    Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());

    if (data.dataPath() == "/") {
        setRelayFromJSON(data.jsonObject());
    } else {
        setRelay(data.dataPath(), data.boolData());
    }
}

void streamTimeoutCallback(bool timeout) {
    if (timeout)
        Serial.println("stream timed out, resuming...\n");

    if (!stream.httpConnected())
        Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}


void loop() {
    timer.run();
    wCre.run();
    server.handleClient();
    syncFBTask.run();
}