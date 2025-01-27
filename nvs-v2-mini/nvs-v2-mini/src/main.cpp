#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <ElegantOTA.h>

#include <LittleFS.h>

#include "GenericOutput.h"
#include "GenericInput.h"
#include "config.h"
#include "MAINPAGE.h"
#include "Timer.h"
#include "ENVFile.h"

#include <IRremote.hpp>

#define FIRMWARE_VERSION 1

GenericOutput relay(RELAY_PIN, RELAY_ACTIVE_STATE);
GenericInput door(DOOR_SENSOR_PIN, INPUT, DOOR_SENSOR_ACTIVE_STATE);
GenericInput pir(PIR_SENSOR_PIN, INPUT_PULLUP, PIR_SENSOR_ACTIVE_STATE);

WiFiEventHandler wifiConnectedHandler;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

ENVFile env;

uint32_t lastMotionTime = 0;
uint32_t motionTimeout = 15000;
bool isInRoom = false;

Timer timer;
int roomOffTimer = -1;

void IRControl(bool state) {
  if (state) {
    IrSender.sendNECRaw(0xFF00FF00, 0);
    timer.setTimeout(1000L, []() {
      IrSender.sendNECRaw(0xF50AFF00, 0);
    });
  } else {
    IrSender.sendNECRaw(0xFF00FF00, 10);
  }
}

void clearRoomOffTimer() {
  if (roomOffTimer != -1) {
    timer.deleteTimer(roomOffTimer);
    roomOffTimer = -1;
  }
}

void roomOn() {
  if (!relay.getState()) {
    IRControl(true);
  }
  relay.on();
  clearRoomOffTimer();
}

void roomOff() {
  if (isInRoom) return;
  IRControl(false);
  relay.off();
}

void startRoomOffTimer() {
  lastMotionTime = millis();
  if (roomOffTimer != -1) {
    timer.deleteTimer(roomOffTimer);
  }
  roomOffTimer = timer.setTimeout(motionTimeout, roomOff);
}

void broadcastState(String var, String state) {
  ws.textAll(var + ":" + state + "\r\n");
}

void broadcastAction(String var, String action) {
  ws.textAll(var + ":ACTION_" + action + "\r\n");
}

void setup() {
  env.begin();
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("Connecting to WiFi...");

  motionTimeout = env.getInt("MOTION_TIMEOUT", motionTimeout);

#ifdef DEVICE_NAME
  WiFi.hostname(DEVICE_NAME);
  MDNS.begin(DEVICE_NAME);
  MDNS.addService("http", "tcp", 80);
#endif
  WiFi.begin("Home", "Imper@thai#01");
  wifiConnectedHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
    Serial.println("=====================================");
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.printf("http://%s\n", WiFi.localIP().toString().c_str());
#ifdef DEVICE_NAME
    Serial.printf("http://%s.local\n", DEVICE_NAME);
#endif
    Serial.println("=====================================");
  });

  IrSender.begin(IR_PIN);

  door.setActiveStateString("CLOSED");
  door.setInactiveStateString("OPENED");
  door.onActive([]() {
    // Door is closed
    isInRoom = pir.getState() || (millis() - lastMotionTime < motionTimeout);
    if (isInRoom) {
      roomOn();
    }
  });
  door.onInactive([]() {
    // Door is opened
    isInRoom = false;
    startRoomOffTimer();
  });
  door.onChange([]() {
    broadcastState("DOOR", door.getStateString());
  });

  pir.setActiveStateString("MOTION");
  pir.setInactiveStateString("NO MOTION");
  pir.onChange([]() {
    broadcastState("PIR", pir.getStateString());
  });
  pir.onActive([]() {
    roomOn();
    if (door.getState()) {
      // Door is closed
      isInRoom = true;
    }
  });
  pir.onInactive([]() {
    startRoomOffTimer();
  });

  relay.onPowerChanged([]() {
    broadcastState("RELAY", relay.getStateString());
  });

  ws.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    (void)len;
    if (type == WS_EVT_CONNECT) {
      IPAddress ip = client->remoteIP();
      Serial.printf("ws[%s][%u] connect\n", ip.toString().c_str(), client->id());
      broadcastState("Firmware version", String(FIRMWARE_VERSION));
      broadcastState("MOTION_TIMEOUT", String(motionTimeout));
      broadcastState("DOOR", door.getStateString());
      broadcastState("PIR", pir.getStateString());
      broadcastState("RELAY", relay.getStateString());
      broadcastAction("RELAY", "ONOFF");
      broadcastAction("MOTION_TIMEOUT", "SET_NUMBER");
    } else if (type == WS_EVT_DISCONNECT) {
      IPAddress ip = client->remoteIP();
      Serial.printf("ws[%s][%u] disconnect\n", ip.toString().c_str(), client->id());
    } else if (type == WS_EVT_ERROR) {
      Serial.println("ws error");
    } else if (type == WS_EVT_PONG) {
      Serial.println("ws pong");
    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len) {
        if (info->opcode == WS_TEXT) {
          data[len] = 0;
          String msg = (char*)data;
          // Serial.printf("ws[%u][msg]: %s\n", client->id(), msg.c_str());
          if (msg.indexOf("RELAY") > -1) {
            if (msg.indexOf("ON") > -1) {
              relay.on();
            } else if (msg.indexOf("OFF") > -1) {
              relay.off();
            }
          } else if (msg.indexOf("MOTION_TIMEOUT") > -1) {
            int timeout = msg.substring(msg.indexOf(":") + 1).toInt();
            if (timeout > 0) {
              motionTimeout = timeout;
              env.set("MOTION_TIMEOUT", (long)timeout);
              broadcastState("MOTION_TIMEOUT", String(motionTimeout));
            }
          }
        }
      }
    }
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", MAINPAGE);
  });

  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Restarting...");
    timer.setTimeout(2000, []() {
      ESP.restart();
    });
  });

  ElegantOTA.begin(&server);

  server.addHandler(&ws);
  server.begin();
}

void loop() {
  MDNS.update();
  ElegantOTA.loop();
  timer.loop();
}