#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <SimpleTimer.h>
#include <FirebaseESP8266.h>
#endif

#include <ESP8266WebServer.h>
#include <ElegantOTA.h>

#include "nhtCredential.h"

const uint8_t C1_PIN = 4;
const uint8_t C2_PIN = 5;
const uint8_t C3_PIN = 16;
const uint8_t C1_BTN_PIN = 14;
const uint8_t C2_BTN_PIN = 12;
const uint8_t C3_BTN_PIN = 13;

const uint8_t TRIGGER_STATE = 0;

bool btnState = false;
bool C1_STATE = false;
bool C2_STATE = false;
bool C3_STATE = false;

#define FIRMWARE_VERSION 2

FirebaseData fbdo;
FirebaseData stream;
SimpleTimer timer;
ESP8266WebServer server(80);
nhtCredential wCre;

void afterOnline();
void firstRun();
String getState(String ch);
void reverseState(String ch, bool v, bool &bState);
void setRelay(String ch, bool state, uint8_t &cb);
bool setRelay(String ch, bool state);
String sync(String ch[], String v[], uint8_t len);
String sync(String ch, String v);
void syncFirebase();
void lightOff();
void streamCb(StreamData stream);
void streamTimeoutCallback(bool timeout); 
void mainLoop();

void setup() {

  pinMode(C1_BTN_PIN, INPUT_PULLUP);
  pinMode(C2_BTN_PIN, INPUT_PULLUP);
  pinMode(C3_BTN_PIN, INPUT_PULLUP);
  pinMode(C1_PIN, OUTPUT);
  setRelay("c1", C1_STATE);
  pinMode(C2_PIN, OUTPUT);
  setRelay("c2", C2_STATE);
  pinMode(C3_PIN, OUTPUT);
  setRelay("c3", C3_STATE);

  Serial.begin(9600);
  while(!Serial) delay(20);

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  ElegantOTA.begin(&server);

  server.on("/firmware", HTTP_ANY, [&]()
            { server.send(200, "text/plain", String(FIRMWARE_VERSION)); });

  String AP_SSID = "Den Phong Tho";
  String AP_PASSWORD = "";

  wCre.connectedCallback(afterOnline);
  wCre.begin(&server, &AP_SSID, &AP_PASSWORD);

}

void afterOnline()
{
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

  timer.setInterval(100L, mainLoop);
  timer.setTimeout(1000L, firstRun);
}

void firstRun(){
  if (Firebase.ready())
  {
    FirebaseJson json;
    json.set("localIP", WiFi.localIP().toString());
    json.set("lastOnline/.sv", "timestamp");
    Firebase.updateNodeSilentAsync(fbdo, "/den_phong_tho", json);

    FirebaseJsonData c1;
    FirebaseJsonData c2;
    FirebaseJsonData c3;
    String ch[3] = {"c1", "c2", "c3"};
    String v[3] = {"", "", ""};
    Firebase.getJSON(fbdo, "/den_phong_tho/all", &json);
    json.get(c1, "c1");
    if (c1.success)
      v[0] = c1.stringValue;
    json.get(c2, "c2");
    if (c2.success)
      v[1] = c2.stringValue;
    json.get(c3, "c3");
    if (c3.success)
      v[2] = c3.stringValue;

    if(v[0] != "" || v[1] != "" || v[2] != "")
      sync(ch, v, 3);
    
  }else{
    timer.setTimeout(1000L, firstRun);
  }
}

bool str2bool(String str)
{
  str.toLowerCase();
  if (str == "true" || str == "on" || str == "high" || str == "1")
    return true;
  // else if(str == "false" || str == "off" || str == "low" || str == "0" || str == "")
  else
    return false;
}

String getState(String ch)
{
  if (ch == "c1")
    return C1_STATE ? "on" : "off";
  if (ch == "c2")
    return C2_STATE ? "on" : "off";
  if (ch == "c3")
    return C3_STATE ? "on" : "off";
  return "Get fail";
}

void reverseState(String ch, bool v, bool &bState)
{
  String st = v ? "on" : "off";
  if (!bState)
  {
    bState = true;
    sync(ch, st);
  }
}

void setRelay(String ch, bool state, uint8_t &cb)
{
  if (ch == "c1")
  {
    if (state != C1_STATE)
      cb++;
    C1_STATE = state;
    digitalWrite(C1_PIN, !C1_STATE);
  }
  if (ch == "c2")
  {
    if (state != C2_STATE)
      cb++;
    C2_STATE = state;
    digitalWrite(C2_PIN, C2_STATE);
  }
  if (ch == "c3")
  {
    if (state != C3_STATE)
      cb++;
    C3_STATE = state;
    digitalWrite(C3_PIN, !C3_STATE);
  }
  if (ch == "all")
  {
    if (state != C1_STATE || state != C2_STATE || state != C3_STATE)
      cb++;
    C1_STATE = state;
    C2_STATE = state;
    C3_STATE = state;
    digitalWrite(C1_PIN, !C1_STATE);
    digitalWrite(C2_PIN, C2_STATE);
    digitalWrite(C3_PIN, !C3_STATE);
  }
}

bool setRelay(String ch, bool state)
{
  uint8_t cb = 0;
  setRelay(ch, state, cb);
  return cb > 0;
}

void lightOff(){
  sync("all", "off");
}

String sync(String ch[], String v[], uint8_t len)
{
  uint8_t isSuccess = 0;
  uint8_t anyChanges = 0;
  for (uint8_t i = 0; i < len; i++)
  {
    isSuccess++;

    if (v[i] == "restart")
      ESP.restart();
    else if (ch[i] != "" && v[i] != "")
      setRelay(ch[i], str2bool(v[i]), anyChanges);
    else
      isSuccess--;
  }
  if (isSuccess > 0 && anyChanges > 0)
  {
    timer.setTimeout(500L, syncFirebase);
    return "success";
  }
  else
    return "fail";
}

String sync(String ch, String v)
{
  String CH[1] = {ch};
  String V[1] = {v};
  return sync(CH, V, 1);
}

void syncFirebase()
{
  if (Firebase.ready())
  {
    FirebaseJson json;
    json.set("c1", C1_STATE);
    json.set("c2", C2_STATE);
    json.set("c3", C3_STATE);
    Firebase.updateNodeSilentAsync(fbdo, "/den_phong_tho/all", json);
  }
}

void streamCb(StreamData data)
{
  Serial.printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  Serial.println();
  Serial.printf("Received stream string:  %s\n", data.stringData().c_str());
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());

  if (data.dataPath() == "/")
  {
    String ch[3] = {"c1", "c2", "c3"};
    String v[3] = {"", "", ""};
    FirebaseJson json = data.jsonObject();
    FirebaseJsonData c1;
    FirebaseJsonData c2;
    FirebaseJsonData c3;
    json.get(c1, "c1");
    if (c1.success)
      v[0] = c1.stringValue;
    json.get(c2, "c2");
    if (c2.success)
      v[1] = c2.stringValue;
    json.get(c3, "c3");
    if (c3.success)
      v[2] = c3.stringValue;
    
    if (v[0] != "" || v[1] != "" || v[2] != "")
      sync(ch, v, 3);
  }
  else
  {
    if (data.dataPath() == "/c1")
      sync("c1", data.stringData());
    else if (data.dataPath() == "/c2")
      sync("c2", data.stringData());
    else if (data.dataPath() == "/c3")
      sync("c3", data.stringData());
  }
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}

void mainLoop(){
  if (digitalRead(C1_BTN_PIN) == TRIGGER_STATE)
    reverseState("c1", !C1_STATE, btnState);
  else if (digitalRead(C2_BTN_PIN) == TRIGGER_STATE)
    reverseState("c2", !C2_STATE, btnState);
  else if (digitalRead(C3_BTN_PIN) == TRIGGER_STATE)
    reverseState("c3", !C3_STATE, btnState);
  else
    btnState = false;
}


void loop() {
  timer.run();
  wCre.run();
  server.handleClient();
}