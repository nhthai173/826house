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
#include "nhtNeoPixel.h"
#include "nhtCredential.h"

#define FIRMWARE_VERSION 1

FirebaseData fbdo;
FirebaseData stream;
SimpleTimer timer;
ESP8266WebServer server(80);
nhtCredential wCre;

/*
  - C1: Light Inside
  - C2: Light Outside
  - C3: Sensor Power
  - C4: Siren
*/

bool C1_STATE = false;
bool C2_STATE = false;
bool C3_STATE = false;
bool C4_STATE = false;
bool SENSOR_STATE = false;
bool ARM_STATE = false;
bool ALARM_STATE = false;
String STATE = "READY";
bool btnState = false;
bool AFTER_SETUP = false;
const uint8_t TRIGGER_STATE = 0;
const uint8_t ARM_BTN_PIN = 5;
const uint8_t RELAY_BTN_PIN = 4;
const uint8_t SENSOR_PIN = 14;
const uint8_t C1_PIN = 16;
const uint8_t C2_PIN = 12;
const uint8_t C3_PIN = 2;
const uint8_t C4_PIN = 13;

const uint8_t LED_NUM = 8;
const uint8_t LED_PIN = 15;
nhtNeoPixel ledStrip(LED_NUM, LED_PIN);

int syncFireaseDelayTimer = -1;
int c3StateTimer = -1;
unsigned long passedTime = 0;

void firstRun();
void afterOnline();
String getState(String ch);
void btnPress(String ch, bool &bState);
bool setRelay(String ch, bool state);
void setRelay(bool &var, bool val, uint8_t &r);
void setLED();
void ledOff();
void c3Timer(String ch, bool state, unsigned long delay);
void c3Off();
bool arm(bool state);
void sensorTrigger(uint8_t state);
bool alarm(bool state, bool force = false);
void updateState();
void updateAlarmState();
String sync(String ch[], String v[], uint8_t len);
void syncFirebase();
void syncFirebaseDelay();
void syncFirebaseDelayCB();
void mainLoop();
void streamCb(StreamData stream);
void streamTimeoutCallback(bool timeout);

const char MAINPAGE[] PROGMEM = R"=====(
Main Page
)=====";

void setup()
{

  pinMode(ARM_BTN_PIN, INPUT_PULLUP);
  pinMode(RELAY_BTN_PIN, INPUT_PULLUP);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(C1_PIN, OUTPUT);
  setRelay("c1", C1_STATE);
  pinMode(C2_PIN, OUTPUT);
  setRelay("c2", C2_STATE);
  pinMode(C3_PIN, OUTPUT);
  setRelay("c3", C3_STATE);
  pinMode(C4_PIN, OUTPUT);
  setRelay("c4", C4_STATE);

  Serial.begin(9600);
  while(!Serial) delay(20);

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  ledStrip.begin();
  ledOff();
  ElegantOTA.begin(&server);

  server.on("/", HTTP_ANY, [&]()
            { server.send_P(200, "text/html", (const char *)MAINPAGE, sizeof(MAINPAGE) / sizeof(MAINPAGE[0])); });

  server.on("/set", HTTP_ANY, [&]()
            {
        String messages = "";
        String ch[] = {""};
        String v[] = {""};
        for (uint8_t i = 0; i < server.args(); i++)
        {
            if (server.argName(i) == "ch")
                ch[0] = server.arg(i);
            if (server.argName(i) == "v")
                v[0] = server.arg(i);
        }
        if (ch[0] == "" && v[0] == "")
          messages = "Invalid!\n/set?channel=[channel]&value=[value]\n[channel]: \"c1\", \"c2\", \"c3\", \"c4\", \"arm\", \"all\", \"alarm\"\n[value] : \"on\", \"off\", \"restart\"";
        else if(ch[0] != "" && v[0] != "")
            messages = sync(ch, v, 1);
        server.send(200, "text/plain", messages); });

  server.on("/get", HTTP_ANY, [&]()
            {
        String messages = "";
        String ch = "";
        for (uint8_t i = 0; i < server.args(); i++)
        {
            if (server.argName(i) == "ch")
                ch = server.arg(i);
        }
        if (ch == "")
          messages = "Invalid!\n/get?channel=[channel]\n[channel]: \"c1\", \"c2\", \"c3\", \"c4\", \"sensor\", \"arm\", \"alarm\", \"state\"";
        else
            messages = getState(ch);
        server.send(200, "text/plain", messages); });

  server.on("/firmware", HTTP_ANY, [&]()
            { server.send(200, "text/plain", String(FIRMWARE_VERSION)); });

  String AP_SSID = "Bao Dong Nha Truoc";
  String AP_PASSWORD = "";

  wCre.connectedCallback(afterOnline);
  wCre.begin(&server, &AP_SSID, &AP_PASSWORD);

  AFTER_SETUP = true;
}

void afterOnline()
{
  Serial.println("Connecting Firebase");

  Firebase.begin(wCre.getDBURL().c_str(), wCre.getDBKEY().c_str());
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);

#if defined(ESP8266)
  stream.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);
#endif

  if (!Firebase.beginStream(stream, "/security_nha_truoc/all"))
    Serial.printf("stream begin error, %s\n\n", stream.errorReason().c_str());
  Firebase.setStreamCallback(stream, streamCb, streamTimeoutCallback);

  timer.setInterval(100L, mainLoop);
  timer.setTimeout(1000L, firstRun);
}

void firstRun()
{
  if (Firebase.ready())
  {
    FirebaseJson json;
    json.set("localIP", WiFi.localIP().toString());
    json.set("lastOnline/.sv", "timestamp");
    Firebase.updateNodeSilentAsync(fbdo, "/security_nha_truoc", json);

    FirebaseJsonData c1;
    FirebaseJsonData c2;
    FirebaseJsonData c3;
    FirebaseJsonData c4;
    FirebaseJsonData arm;
    String ch[5] = {"c1", "c2", "c3", "c4", "arm"};
    String v[5] = {"", "", "", "", ""};
    Firebase.getJSON(fbdo, "/security_nha_truoc/all", &json);
    json.get(c1, "c1");
    if (c1.success)
      v[0] = c1.stringValue;
    json.get(c2, "c2");
    if (c2.success)
      v[1] = c2.stringValue;
    json.get(c3, "c3");
    if (c3.success)
      v[2] = c3.stringValue;
    json.get(c4, "c4");
    if (c4.success)
      v[3] = c4.stringValue;
    json.get(arm, "arm");
    if(arm.success)
      v[4] = arm.stringValue;
    
    if (v[0] == "restart"){
      v[0] = "off";
      C1_STATE = true;
    }
    if (v[1] == "restart"){
      v[1] = "off";
      C2_STATE = true;
    }
    if (v[2] == "restart"){
      v[2] = "off";
      C3_STATE = true;
    }
    if (v[3] == "restart"){
      v[3] = "off";
      C4_STATE = true;
    }
    if (v[4] == "restart"){
      v[4] = "off";
      ARM_STATE = true;
    }

    if (v[0] != "" || v[1] != "" || v[2] != "" || v[3] != "" || v[4] != "")
      sync(ch, v, 5);
  }
  else
  {
    timer.setTimeout(1000L, firstRun);
  }
}

void ledOff(){
  ledStrip.off();
}

void setLED(){
  if(C3_STATE){
    if(STATE == "NOT READY")
      ledStrip.setRGB(255, 255, 0);
    if(STATE == "ARMED")
      ledStrip.setRGB(255, 0, 0);
    if(STATE == "ALARM" || ALARM_STATE)
      ledStrip.policeMode();
    if(STATE == "READY"){
      ledStrip.setRGB(0, 255, 0);
      /*if(readyStateTimer < 0)
        readyStateTimer = timer.setTimeout(10000L, ledOff);
    }else if(readyStateTimer >= 0){
      timer.deleteTimer(readyStateTimer);
      readyStateTimer = -1;*/
    }
  }else{
    ledOff();
  }
}

bool str2bool(String str){
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
  if (ch == "c4")
    return C4_STATE ? "on" : "off";
  if (ch == "arm")
    return ARM_STATE ? "on" : "off";
    //return ARM_STATE ? "armed" : "disarmed";
  if (ch == "sensor")
    return SENSOR_STATE ? "on" : "off";
    //return SENSOR_STATE ? "not ready" : "ready";
  if (ch == "all"){
    return "c1: " + String(C1_STATE) + "\r\nc2: " + String(C2_STATE) + "\r\nc3: " + String(C3_STATE) + "\r\nc4: " + String(C4_STATE) + "\r\nsensor: " + String(SENSOR_STATE) + "\r\narm: " + String(ARM_STATE) + "\r\nalarm: " + String(ALARM_STATE) + "\r\nstate: " + STATE;
  }
  return "Get fail";
}

void btnPress(String ch, bool &bState)
{
  if(!bState){
    String cha[2] = {"", ""};
    String v[2] = {"on", "on"};
    uint8_t len = 1;
    if (ch == "relay")
    {
      if (!C1_STATE && !C2_STATE)
        cha[0] = "c1";
      else if (C1_STATE && !C2_STATE)
        cha[0] = "c2";
      else if (C1_STATE && C2_STATE)
      {
        cha[0] = "c1";
        v[0] = "off";
      }
      else if (!C1_STATE && C2_STATE)
      {
        cha[0] = "c2";
        v[0] = "off";
      }
    }
    else if (ch == "arm")
    {
      if (ALARM_STATE && ARM_STATE && SENSOR_STATE)
      {
        cha[0] = "arm";
        v[0] = "off";
      }
      else if (ALARM_STATE && C4_STATE)
      {
        cha[0] = "c4";
        v[0] = "off";
      }
      else if (ALARM_STATE)
      {
        cha[0] = "alarm";
        v[0] = "off";
      }
      else if (!ARM_STATE)
      {
        if (!C3_STATE)
          cha[0] = "c3";
        else
          cha[0] = "arm";
      }
      else if (ARM_STATE)
      {
        cha[0] = "arm";
        v[0] = "off";
        cha[1] = "c3";
        v[1] = "off";
        len = 2;
      }
    }
    if (cha[0] != "")
      sync(cha, v, len);

    bState = true;
  }
}

bool setRelay(String ch, bool state){
  bool r = false;
  if (ch == "c1")
  {
    if (state != C1_STATE)
      r = true;
    C1_STATE = state;
    digitalWrite(C1_PIN, !C1_STATE);
  }
  if (ch == "c2")
  {
    if (state != C2_STATE)
      r = true;
    C2_STATE = state;
    digitalWrite(C2_PIN, !C2_STATE);
  }
  if (ch == "c3")
  {
    if (state != C3_STATE)
      r = true;
    C3_STATE = state;
    digitalWrite(C3_PIN, !C3_STATE);
    if(AFTER_SETUP)
      c3Timer("c3", state, 1000L);
  }
  if (ch == "c4")
  {
    if (state != C4_STATE)
      r = true;
    C4_STATE = state;
    digitalWrite(C4_PIN, !C4_STATE);
  }
  if (ch == "all")
  {
    if (state != C1_STATE || state != C2_STATE)
      r = true;
    C1_STATE = state;
    C2_STATE = state;
    digitalWrite(C1_PIN, !C1_STATE);
    digitalWrite(C2_PIN, !C2_STATE);
  }
  if (ch == "arm" && state != ARM_STATE)
    r = arm(state);
  if (ch == "alarm" && !state)
    r = alarm(false, true);
  return r;
}

void setRelay(String ch, bool state, uint8_t &cb){
  if(setRelay(ch, state))
    cb++;
}

void c3Timer(String ch, bool state, unsigned long delay = 0){
  if ((ch == "c3" && state) || (ch == "arm" && !state))
  {
    if (c3StateTimer >= 0)
      timer.deleteTimer(c3StateTimer);
    c3StateTimer = timer.setTimeout(300000L, c3Off);
  }
  else if (c3StateTimer >= 0 && ( (ch == "c3" && !state) || (ch == "arm" && state) ))
  {
    timer.deleteTimer(c3StateTimer);
    c3StateTimer = -1;
  }
  if(delay > 0){
    timer.setTimeout(delay, updateState);
  }else
    updateState();
}

void c3Off(){
  String ch[1] = {"c3"};
  String v[1] = {"off"};
  if(!ARM_STATE){
    sync(ch, v, 1);
    setLED();
  }
  c3StateTimer = -1;
}

bool arm(bool state){
  bool r = false;
  if(state != ARM_STATE){
    ARM_STATE = state;
    if(!ARM_STATE)
      r = alarm(false);
    else{
      if (!C3_STATE){
        r = setRelay("c3", true);
        delay(500);
      }
      sensorTrigger(digitalRead(SENSOR_PIN));
      if(SENSOR_STATE){
        r = false;
        ARM_STATE = false;
        // thong bao cam bien dang vuong
      }
    }
    c3Timer("arm", state);
  }
  return r;
}

bool alarm(bool state, bool force){
  bool anyChange = setRelay("c4", state);
  if (state || force){
    bool a1 = setRelay("c1", state);
    bool a2 = setRelay("c2", state);
    if(a1 || a2)
      anyChange = true;
  }
  /*if(anyChange)
    syncFirebaseDelay();*/
  if(state != ALARM_STATE){
    ALARM_STATE = state;
    setLED();
    timer.setTimeout(500L, updateAlarmState);
  }
  return anyChange;
}

void sensorTrigger(uint8_t state){
  bool st = false;
  if(state == TRIGGER_STATE){
    st = true;
    if(C3_STATE)
      passedTime = millis();
  }
  if(st != SENSOR_STATE){
    SENSOR_STATE = st;
    c3Timer("c3", true);
  }
}

void updateState(){
  String newState = "";
  if(SENSOR_STATE && !ARM_STATE)
    newState = "NOT READY";
  else if (SENSOR_STATE && ARM_STATE){
    newState = "ALARM";
    alarm(true);
  }else if (!SENSOR_STATE && ARM_STATE)
    newState = "ARMED";
  else if (!SENSOR_STATE && !ARM_STATE)
    newState = "READY";
  if(newState != "" && newState != STATE){
    STATE = newState;
    setLED();
    syncFirebaseDelay();
  }
}

String sync(String ch[], String v[], uint8_t len)
{
  uint8_t isSuccess = 0;
  uint8_t anyChanges = 0;
  for(uint8_t i=0; i<len; i++){
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

void syncFirebaseDelay(){
  if(syncFireaseDelayTimer >= 0)
    timer.deleteTimer(syncFireaseDelayTimer);
  syncFireaseDelayTimer = timer.setTimeout(2000L, syncFirebaseDelayCB);
}

void syncFirebaseDelayCB(){
  syncFireaseDelayTimer = -1;
  if(Firebase.ready()){
    Firebase.setStringAsync(fbdo, "/security_nha_truoc/state", STATE);
    syncFirebase();
  }
}

void updateAlarmState(){
  if (Firebase.ready())
    Firebase.setBoolAsync(fbdo, "/den_nha_truoc/all/alarm", ALARM_STATE);
}

void syncFirebase()
{
  if (Firebase.ready())
  {
    FirebaseJson json;
    FirebaseJson json1;
    json.set("c1", C1_STATE);
    json.set("c2", C2_STATE);
    json.set("c3", C3_STATE);
    json.set("c4", C4_STATE);
    json.set("arm", ARM_STATE);
    Firebase.updateNodeSilentAsync(fbdo, "/security_nha_truoc/all", json);
    if(passedTime > 0){
      long map = millis() - passedTime;
      json1.set("time/.sv", "timestamp");
      json1.set("map", map);
      passedTime = 0;
      Firebase.pushJSONAsync(fbdo, "/security_nha_truoc/log", json1);
    }
  }
}

void mainLoop()
{
  sensorTrigger(digitalRead(SENSOR_PIN));

  if (digitalRead(ARM_BTN_PIN) == TRIGGER_STATE)
    btnPress("arm", btnState);
  else if(digitalRead(RELAY_BTN_PIN) == TRIGGER_STATE)
    btnPress("relay", btnState);
  else
    btnState = false;
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
    String ch[5] = {"c1", "c2", "c3", "c4", "arm"};
    String v[5] = {"", "", "", "", ""};
    FirebaseJson json = data.jsonObject();
    FirebaseJsonData c1;
    FirebaseJsonData c2;
    FirebaseJsonData c3;
    FirebaseJsonData c4;
    FirebaseJsonData arm;
    json.get(c1, "c1");
    if (c1.success)
      v[0] = c1.stringValue;
    json.get(c2, "c2");
    if (c2.success)
      v[1] = c2.stringValue;
    json.get(c3, "c3");
    if (c3.success)
      v[2] = c3.stringValue;
    json.get(c4, "c4");
    if (c4.success)
      v[3] = c4.stringValue;
    json.get(arm, "arm");
    if (arm.success)
      v[4] = arm.stringValue;
    if (v[0] != "" || v[1] != "" || v[2] != "" || v[3] != "" || v[4] != "")
      sync(ch, v, 5);
  }
  else
  {
    String ch[1] = {""};
    String v[1] = {data.stringData()};

    if (data.dataPath() == "/c1")
      ch[0] = "c1";
    else if(data.dataPath() == "/c2")
      ch[0] = "c2";
    else if (data.dataPath() == "/c3")
      ch[0] = "c3";
    else if (data.dataPath() == "/c4")
      ch[0] = "c4";
    else if(data.dataPath() == "/arm")
      ch[0] = "arm";
    if(ch[0] != "")
      sync(ch, v, 1);
  }

}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}

void loop()
{
  timer.run();
  wCre.run();
  ledStrip.run();
  server.handleClient();
}