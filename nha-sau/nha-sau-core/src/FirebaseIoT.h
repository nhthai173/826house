//
// Created by Thái Nguyễn on 15/7/24.
//

#ifndef NHA_SAU_CORE_FIREBASEIOT_H
#define NHA_SAU_CORE_FIREBASEIOT_H

#include <Firebase_ESP_Client.h>
#include <addons/RTDBHelper.h>
#include "base64.hpp"
#include "GenericOutput.h"

#define DEBUG

#if defined(DEBUG)
#define D_Print(...) Serial.printf(__VA_ARGS__)
#else
#define D_Print(...)
#endif

// Firebase Objects
FirebaseData fbStream;
FirebaseData fbdo;
FirebaseConfig fbConfig;
FirebaseAuth fbAuth;

String DB_DEVICE_PATH = "";
uint32_t fb_last_millis = 0;
bool FB_FLAG_FIRST_INIT = false;


void fb_setup(const String &email, const String &password, const String &api_key, const String &database_url) {
    fbAuth.user.email = email;
    fbAuth.user.password = password;
    fbConfig.api_key = api_key;
    fbConfig.database_url = database_url;
    fbConfig.timeout.serverResponse = 10 * 1000;

    Firebase.reconnectNetwork(true);
    fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */,
                           1024 /* Tx buffer size in bytes from 512 - 16384 */);
    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);
    Firebase.setDoubleDigits(5);
    Firebase.begin(&fbConfig, &fbAuth);
}


String fb_get_uid(String token) {
    if (token.length() == 0) {
        return "";
    }
    String data = token.substring(token.indexOf('.') + 1, token.lastIndexOf('.'));
    if (data.length() == 0) {
        return "";
    }
    unsigned char *data_decoded = new unsigned char[data.length() + 1];
    std::strncpy((char*)data_decoded, data.c_str(), data.length());
    data_decoded[data.length()] = '\0';
    unsigned char string[800];
    unsigned int string_len = decode_base64(data_decoded, string);
    delete[] data_decoded;
    if (string_len == 0) {
        return "";
    }
    String data_decoded_str = String((char*)string);
    int uid_index = data_decoded_str.indexOf("\"user_id\":\"");
    if (uid_index == -1)
        return "";
    return data_decoded_str.substring(uid_index + 11, data_decoded_str.indexOf("\"", uid_index + 11));
}


void fb_setStreamCallback(FirebaseData::StreamEventCallback dataAvailableCallback) {
    Firebase.RTDB.setStreamCallback(&fbStream, dataAvailableCallback, nullptr);
}

void fb_setStreamCallback(FirebaseData::StreamEventCallback dataAvailableCallback, FirebaseData::StreamTimeoutCallback timeoutCallback) {
    Firebase.RTDB.setStreamCallback(&fbStream, dataAvailableCallback, timeoutCallback);
}

void fb_loop() {
    // First init
    if (!FB_FLAG_FIRST_INIT && Firebase.ready()) {
        FB_FLAG_FIRST_INIT = true;

        // Set device path
        String token = Firebase.getToken();
        if (token.length() == 0) {
            D_Print("Token not found\n");
            return;
        }
        String uid = fb_get_uid(token);
        if (uid.length() == 0) {
            D_Print("UID not found\n");
            return;
        }
        DB_DEVICE_PATH = "/" + uid + "/" + ESP.getChipId();
        D_Print("Device path: %s\n", DB_DEVICE_PATH.c_str());

        // Set stream
        if (!Firebase.RTDB.beginStream(&fbStream, DB_DEVICE_PATH + "/data")) {
            Serial.printf("stream begin error, %s\n\n", fbStream.errorReason().c_str());
        }

        fb_last_millis = millis();
    }

    // Check token expired every 10 minutes
    if (FB_FLAG_FIRST_INIT && millis() - fb_last_millis > 10 * 60 * 1000) {
        fb_last_millis = millis();
        Firebase.ready();
    }
}

void fb_syncState(GenericOutput *dev, const String &path) {
    dev->onPowerChanged([dev, path]() {
        schedule_function([dev, path]() {
            D_Print("Sync state %s - %s\n", path.c_str(), dev->getStateString().c_str());
            Firebase.RTDB.setBool(&fbdo, DB_DEVICE_PATH + "/data" + path, dev->getState());
        });
    });
}

#endif //NHA_SAU_CORE_FIREBASEIOT_H
