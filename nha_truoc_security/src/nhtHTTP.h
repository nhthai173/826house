#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

String httpGet(String url){
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    HTTPClient http;
    //client->setFingerprint(fingerprint);
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
    // client->setInsecure();
    if (http.begin(*client, url)){
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            return http.getString();
    }
    return "";
}