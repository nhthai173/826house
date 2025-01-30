#include "nhtCredential.h"

const uint32_t WEBPAGE_SIZE = 4292;
const char WEBPAGE[] PROGMEM = R"=====(
<html><meta name="viewport" content="width=device-width,initial-scale=1"><title>Config</title><head><style>.header{margin-top:90px;font-size:26px;font-weight:500;text-align:center;width:100vw}#header{color:#1abc9c}.form{position:relative;width:100vw;max-width:260px;margin:30px auto 0;box-shadow:2px 2px 5px 1px #00000033;padding:5px 20px 40px 20px;border-radius:6px}.f{position:relative;margin:40px 0}#rssi{position:absolute;font-size:16px;top:30px;right:20px}input{width:100%;display:block;border:none;border-bottom:solid 1px #1abc9c;color:#0e6252;font-size:16px}label{position:absolute;left:-.2rem;top:-.2rem;padding:0 .25rem;color:#00000080;transition:.3s;font-size:16px}input:focus,input:valid{box-shadow:none;outline:0}input:focus+label,input:valid+label{top:-1.4rem;z-index:10;color:#1abc9c;font-size:14px}button{border:none;cursor:pointer;border-radius:4px;padding:10px;margin:10px 0;width:100%;color:#fff;font-size:16px}button.a{background:#1abc9c}button.b{background:#1a7bbc}button.c{background:#bc1a1a}button.d{background:#bd93f9}#loading{width:calc(100% + 40px);height:.25rem;position:relative;margin:-5px -20px 0 -20px}#loading:before{content:"";position:absolute;right:auto;left:0;height:100%;background-color:#1abc9c;animation:lineLoading 1s forwards infinite linear}@keyframes lineLoading{0%{right:100%}50%{right:0;left:0}100%{right:0;left:100%}}</style></head><body><script>var il = { "SSID": 0, "Password": 1, "Database Url": 2, "Database Key": 3};
var fG = {};
var bc = '<div class=header>Config <span id="header"></span></div><div class="form"><div id="loading"></div><span id="rssi"></span>'
for (i in il)
    bc += `<div class="f"><input id="${il[i]}" required=""><label>${i}</label></div>`;
document.body.innerHTML += bc + '<button class="a" onclick="credentials_rec()">Submit</button><hr>' + '<a href="/update" onclick="loading()"><button class="d">Update Firmware</button></a><button class="b" onclick="_restart()">Restart</button><button class="c" onclick="_reset()">Reset</button></div>';
function loading(e = "") {
    document.getElementById("loading").style.display = e;
}
function h(u, c) {
    loading();
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4){
            loading("none");
            if(this.status == 200 && c)
                c(this.responseText);
            else if(c)
                c("");
        }
    };
    setTimeout(() => {
        xhttp.open("GET", u, true);
        xhttp.send();
    }, 200);
}
h("/eeprom", function(d) {
    if(d && d.match(/\d/g)) {
        d = d.split("||");
        for (i in d) {
            if (i == d.length - 1)
                document.getElementById("rssi").innerText = d[i];
            else if(i == d.length - 2)
                document.getElementById("header").innerText = d[i];
            else if(d[i]){
                s(i, d[i]);
                fG[i] = d[i];
            }
        }
    }
});
function g(e) { return document.getElementById(e).value; }
function s(e, t) { document.getElementById(e).value = t; }
function p(t) { return t.toLowerCase().match(/\w/g, "").join("") }
function s1w(u) {
    h(u, function (a) {
        alert(a);
    });
}
function s2w(dt, dl) {
    h(dt, function(a) {
        alert(dl);
        location.reload();
    });
}
function credentials_rec() {
    var d = "";
    var r = 0;
    for (i in il){
        var tt = g(il[i]);
        if(!tt){
            alert("Please fill in all field");
            return 0;
        }
        if(fG[il[i]] == tt)
            r++;
        d += p(i) + "=" + encodeURIComponent(tt) + "&";
    }
    if(il.length == r)
        alert("Nothing changes");
    else
        s2w('/update-eeprom?' + d.substring(0, d.length - 1), 'Credentials received by ESP board!!!');
}
function _restart() { s2w('/restart', 'Restart Done!'); }
function _reset() { s2w('/reset', 'Reset Done!'); }</script></body></html>
)=====";

//
void nhtCredential::begin(ESP8266WebServer *server, String *ssidAP, String *passwordAP)
{

    _server = server;

    if (*ssidAP != "")
    {
        _ssidAP = *ssidAP;
        _passwordAP = *passwordAP;
    }

    readEEPROM();

    uint8_t wifi_connect_timeout = 0;
    bool isConnectedWifi = false;
    if (_ssid != "")
    {
        WiFi.begin(_ssid, _password);
        Serial.println("Waiting for Wifi to connect");
        Serial.println();
        while (wifi_connect_timeout < 30)
        {
            if (isConnected() == true)
            {
                isConnectedWifi = true;
                break;
            }
            delay(300);
            Serial.print("*");
            wifi_connect_timeout++;
        }
        if (isConnectedWifi == false){
            Serial.println("Connect timed out");
            setupAP();
        }
    }
    else
        setupAP();

    createWebServer();
    _server->begin();

    if(isConnectedWifi == true){
        Serial.print("Connected with IP: ");
        Serial.println(WiFi.localIP());
        Serial.println();
        if (_connectedCallback != NULL)
            (*_connectedCallback)();
    }

}






//
void nhtCredential::begin(ESP8266WebServer *server, String *ssidAP, String *passwordAP, String *wSSID, String *wPassword, String *fdbURL, String *fdbKEY)
{
    writeEEPROM(wSSID, wPassword, fdbURL, fdbKEY);
    begin(server, ssidAP, passwordAP);
}



//
bool nhtCredential::isConnected(){
    if (WiFi.status() == WL_CONNECTED)
        return true;
    return false;
}


//
void nhtCredential::setupAP()
{
    if(_ssidAP != ""){
        WiFi.disconnect();
        delay(100);
        WiFi.softAP(_ssidAP, _passwordAP);
        Serial.print("Started SoftAP - ");
        Serial.println(_ssidAP);
        _preMillis = millis();
    }else
        Serial.println("Setup AP Fail - AP_ssid is empty");
}






//
void nhtCredential::tryConnect(){
    if (isConnected() == true && _preMillis > 0)
    {
        Serial.println("WiFi Connected");
        WiFi.softAPdisconnect(true);
        _preMillis = 0;
        if (_connectedCallback != NULL)
            (*_connectedCallback)();
    }
    else if (WiFi.softAPgetStationNum() > 0)
        _preMillis = millis();
    else
        ESP.restart();
}



//
void nhtCredential::createWebServer(){

    _server->onNotFound([&]() {
        _server->send(404, "text/plain", "Not found");
    });

    _server->on("/config", HTTP_ANY, [&]() {
        _server->send_P(200, "text/html", (const char*)WEBPAGE, sizeof(WEBPAGE)/sizeof(WEBPAGE[0])); 
    });

    _server->on("/eeprom", HTTP_ANY, [&]() {
        _server->send(200, "text/plain", _ssid+"||"+_password+"||"+_DBurl+"||"+_DBkey+"||"+_ssidAP+"||"+WiFi.RSSI()+ "dBm");
    });

    _server->on("/erase-eeprom", HTTP_ANY, [&]() {
        eraseEEPROM();
        _server->send(200, "text/plain", "success");
    });

    _server->on("/update-eeprom", HTTP_ANY, [&]() {
        String message = "fail";
        String ssid = "";
        String password = "";
        String durl = "";
        String dkey = "";
        for (uint8_t i = 0; i < _server->args(); i++)
        {
            String aName = _server->argName(i);
            if (aName == "ssid")
                ssid = _server->arg(i);
            if (aName == "password")
                password = _server->arg(i);
            if (aName == "databaseurl")
                durl = _server->arg(i);
            if (aName == "databasekey")
                dkey = _server->arg(i);
        }
        if(ssid != "" && durl != "" && dkey != "")
            message = "success";
        _server->send(200, "text/plain", message);
        if(message == "success"){
            writeEEPROM(&ssid, &password, &durl, &dkey);
            ESP.restart();
        }
    });

    _server->on("/restart", HTTP_ANY, [&]() {
        _server->send(200, "text/plain", "restart success");
        delay(1000);
        ESP.restart();
    });

    _server->on("/reset", HTTP_ANY, [&]() {
        _server->send(200, "text/plain", "reset success");
        delay(1000);
        eraseEEPROM();
        ESP.restart();
    });

    _server->on("/format", HTTP_ANY, [&]() {
        _server->send(200, "text/plain", "format success");
        delay(500);
        LittleFS.begin();
        delay(500);
        LittleFS.format();
        delay(500);
        LittleFS.end();
    });

}


// Erase eeprom
void nhtCredential::eraseEEPROM()
{
    EEPROM.begin(512);
    for (int i = 0; i < 300; ++i)
        EEPROM.write(i, 0);
    EEPROM.commit();
}




//
void nhtCredential::readEEPROM()
{
    EEPROM.begin(512);

    
    Serial.print("Read SSID: ");
    for (int i = 0; i < 32; ++i)
    {
        if (char(EEPROM.read(i)) != '\0')
            _ssid += char(EEPROM.read(i));
        else
            break;
    }
    Serial.println(_ssid);

    Serial.print("Read Password: ");
    for (int i = 32; i < 64; ++i)
    {
        if (char(EEPROM.read(i)) != '\0')
            _password += char(EEPROM.read(i));
        else
            break;
    }
    Serial.println(_password);

    Serial.print("Read DB URL: ");
    for (int i = 64; i < 220; ++i)
    {
        if (char(EEPROM.read(i)) != '\0')
            _DBurl += char(EEPROM.read(i));
        else
            break;
    }
    Serial.println(_DBurl);

    Serial.print("Read DB Key: ");
    for (int i = 220; i < 300; ++i)
    {
        if (char(EEPROM.read(i)) != '\0')
            _DBkey += char(EEPROM.read(i));
        else
            break;
    }
    Serial.println(_DBkey);

    EEPROM.commit();
}





//
void nhtCredential::writeEEPROM(String *wSSID, String *wPassword, String *fdbURL, String *fdbKEY){
    if (*wSSID != "" && *fdbURL != "" && *fdbKEY != "")
    {
        eraseEEPROM();
        delay(1000);
        EEPROM.begin(512);
        // ssid
        Serial.print("Write SSID: ");
        Serial1.println(*wSSID);
        for (int i = 0; (unsigned int)i < (*wSSID).length(); ++i)
            EEPROM.write(i, (*wSSID)[i]);
        // password
        Serial.print("Write Password: ");
        Serial.println(*wPassword);
        for (int i = 0; (unsigned int)i < (*wPassword).length(); ++i)
            EEPROM.write(32 + i, (*wPassword)[i]);
        // database url
        Serial.print("Write Database URL: ");
        Serial.println(*fdbURL);
        for (int i = 0; (unsigned int)i < (*fdbURL).length(); ++i)
            EEPROM.write(64 + i, (*fdbURL)[i]);
        // database key
        Serial.printf("Write Database Key: ");
        Serial.println(*fdbKEY);
        for (int i = 0; (unsigned int)i < (*fdbKEY).length(); ++i)
            EEPROM.write(220 + i, (*fdbKEY)[i]);

        EEPROM.commit();
        delay(2000);
    }
}







//
void nhtCredential::connectedCallback(nhtCredential_callback f){
    _connectedCallback = f;
}





//
void nhtCredential::run(){
    if(_preMillis > 0 && millis() - _preMillis >= _tryConnectInterval){
        tryConnect();
    }
}





//
String nhtCredential::getDBURL(){
    return _DBurl;
}




//
String nhtCredential::getDBKEY(){
    return _DBkey;
}



//
String nhtCredential::getSSID(){
    return _ssid;
}



//
String nhtCredential::getPASSWORD(){
    return _password;
}





















/*
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);


//
// This is the webpage which is hosted on your ESP board
// and you can access this webpage by going to this address
//
//                    192.168.4.1
//



// Erase eeprom
void _erase_eeprom()
{
    EEPROM.begin(512); // Initialasing EEPROM
    Serial.println("Clearing eeprom");
    for (byte i = 0; i < 160; ++i)
        EEPROM.write(i, 0);
    EEPROM.commit();
}

void _webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected!\n", num);
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // send message to client
        webSocket.sendTXT(num, "Connected");
    }
    break;
    case WStype_TEXT:
        Serial.printf("[%u] get Text: %s\n", num, payload);
        // if message start with #
        if (payload[0] == '#' && payload[1] == 'R')
        {
            String message = String((char *)(payload));
            message = message.substring(1);
            Serial.println(message);
            if (message == "RESTART")
                ESP.restart();
            else if (message == "RESET")
            {
                _erase_eeprom();
                ESP.restart();
            }
        }
        else if (payload[0] == '#')
        {
            String message = String((char *)(payload));
            message = message.substring(1);
            Serial.println(message);
            // JSON part
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, message);

            String ssid = doc["ssid"];
            String pass = doc["pass"];
            String auth = doc["auth"];
            String domain = doc["domain"];
            String port = doc["port"];

            // Clearing EEPROM
            Serial.println("clearing eeprom");
            for (byte i = 0; i < 160; ++i)
            {
                EEPROM.write(i, 0);
            }

            // Storing in EEPROM
            // ssid
            for (byte i = 0; i < ssid.length(); ++i)
                EEPROM.write(i, ssid[i]);
            // pass
            for (byte i = 0; i < pass.length(); ++i)
                EEPROM.write(32 + i, pass[i]);
            // auth token
            for (byte i = 0; i < auth.length(); ++i)
                EEPROM.write(64 + i, auth[i]);
            // domain
            for (byte i = 0; i < domain.length(); ++i)
                EEPROM.write(100 + i, domain[i]);
            // port
            for (byte i = 0; i < port.length(); ++i)
                EEPROM.write(150 + i, port[i]);

            EEPROM.commit();
            delay(2000);

            // Restarting ESP board
            ESP.restart();
            break;
        }
    }
}

void credentials::Erase_eeprom()
{
    EEPROM.begin(512); // Initialasing EEPROM
    Serial.println("Clearing eeprom");
    for (byte i = 0; i < 160; ++i)
        EEPROM.write(i, 0);
    EEPROM.commit();
}

// Get auth token from eeprom
String credentials::EEPROM_authString()
{
    EEPROM.begin(512); // Initialasing EEPROM
    Serial.println("Reading EEPROM");
    for (byte i = 0; i < 32; ++i)
    {
        if (char(EEPROM.read(i)) != '\0')
            ssid += char(EEPROM.read(i));
        else
            break;
    }
    Serial.print("SSID: ");
    Serial.println(ssid);

    for (int i = 32; i < 64; ++i)
    {
        if (char(EEPROM.read(i)) != '\0')
            pass += char(EEPROM.read(i));
        else
            break;
    }
    Serial.print("Password: ");
    Serial.println(pass);

    for (int i = 64; i < 100; ++i)
    {
        if (char(EEPROM.read(i)) != '\0')
            auth_token += char(EEPROM.read(i));
        else
            break;
    }
    Serial.print("Auth Token: ");
    Serial.println(auth_token);

    return auth_token;
}

// Get blynk server from eeprom
String credentials::EEPROM_blynkServer()
{
    for (int i = 100; i < 150; ++i)
    {
        if (char(EEPROM.read(i)) != '\0')
            blynk_server += char(EEPROM.read(i));
        else
            break;
    }
    Serial.print("Blynk Server: ");
    Serial.println(blynk_server);
    return blynk_server;
}

// Get blynk server port from eeprom
String credentials::EEPROM_blynkPort()
{
    for (int i = 150; i < 160; ++i)
    {
        if (char(EEPROM.read(i)) != '\0')
            blynk_port += char(EEPROM.read(i));
        else
            break;
    }
    Serial.print("Server Port: ");
    Serial.println(blynk_port);
    return blynk_port;
}

// check wifi connect
bool credentials::credentials_get()
{
    if (_testWifi())
    {
        Serial.println("Succesfully Connected!!!");
        return true;
    }
    else
    {
        return false;
    }
}

void credentials::setupAP(char *softap_ssid, char *softap_pass)
{
    WiFi.disconnect();
    delay(100);
    WiFi.softAP(softap_ssid, softap_pass);
    Serial.println("softap");
    _launchWeb();
    Serial.println("Server Started");
    webSocket.begin();
    webSocket.onEvent(_webSocketEvent);
}

// Wifi strength
void credentials::_wStrength()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    int cnt = WiFi.scanNetworks();
    for (int i = 0; i < cnt; i++)
    {
        if (WiFi.SSID(i) == ssid)
        {
            Serial.print(WiFi.SSID(i) + " ");
            ssid_rssi = String(WiFi.RSSI(i)) + "dBm";
            Serial.println(ssid_rssi);
            break;
        }
    }
    delay(100);
}

void credentials::setupWS()
{
    _createWebServer();
    webSocket.begin();
    webSocket.onEvent(_webSocketEvent);
}

void credentials::_reconnectWifi()
{
    char *my_ssid = &ssid[0];
    char *my_pass = &pass[0];
    WiFi.begin(my_ssid, my_pass);
}

bool credentials::_testWifi()
{
    char *my_ssid = &ssid[0];
    char *my_pass = &pass[0];
    WiFi.begin(my_ssid, my_pass);
    int c = 0;
    Serial.println("Waiting for Wifi to connect");
    while (c < 20)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            return true;
        }
        delay(500);
        Serial.print("*");
        c++;
    }
    Serial.println("");
    Serial.println("Connect timed out");
    return false;
}

// This is the function which will be called when an invalid page is called from client
void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void credentials::_createWebServer()
{
    server.on("/", [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", _webpage); });
    server.on("/eeprom", [&](AsyncWebServerRequest *request)
              {
    request->send(200, "text/plain", ssid+"||"+pass+"||"+auth_token+"||"+blynk_server+"||"+blynk_port+"||"+ssid_rssi);
    //+"||"+_wStrength(ssid) });
    server.onNotFound(notFound);
    server.begin();
}

void credentials::_launchWeb()
{
    Serial.println("");
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
    _createWebServer();
}

void credentials::server_loops()
{
    webSocket.loop();
}








*/