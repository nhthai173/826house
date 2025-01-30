#if defined(ESP8266)
    #include "ESP8266WiFi.h"
    #include "WiFiClient.h"
    #include "ESP8266WebServer.h"
#elif defined(ESP32)
    #include "WiFi.h"
    #include "WiFiClient.h"
    #include "WebServer.h"
#endif

#include <EEPROM.h>

typedef void (*nhtCredential_callback)(void);


class nhtCredential
{

public:
    void begin(ESP8266WebServer *server, String *ssidAP, String *passwordAP);
    void begin(ESP8266WebServer *server, String *ssidAP, String *passwordAP, String *wSSID, String *wPassword, String *fdbURL, String *fdbKEY);
    bool isConnected();
    void eraseEEPROM();
    void run();
    String getSSID();
    String getPASSWORD();
    String getDBURL();
    String getDBKEY();
    void connectedCallback(nhtCredential_callback f);
    unsigned long _tryConnectInterval = 60000;

private:
    ESP8266WebServer *_server;
    String _ssid = "";
    String _password = "";
    String _DBurl = "";
    String _DBkey = "";
    String _ssidAP = "";
    String _passwordAP = "";
    unsigned long _preMillis = 0;
    nhtCredential_callback _connectedCallback;
    void createWebServer();
    void readEEPROM();
    void writeEEPROM(String *wSSID, String *wPassword, String *fdbURL, String *fdbKEY);
    void reconnectWifi();
    void setupAP();
    void tryConnect();
};