#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <SoftwareSerial.h>

#ifndef STASSID
#define STASSID "rodrigowifi"
#define STAPSK "3635840331"
#endif

ESP8266WiFiMulti WiFiMulti;
SoftwareSerial nanoSerial(14, 12); // RX (GPIO 12, D5), TX (GPIO 14, D6)

float lastValueSent = -999.0; // Initialize to an impossible value
float lastValueReceived = -999.0; // Initialize to an impossible value
const long interval = 300; // 500 milliseconds

void setup() {
  Serial.begin(9600);
  nanoSerial.begin(9600); // Initialize nanoSerial at 9600 baud rate

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(STASSID, STAPSK);

  Serial.println("Connecting to WiFi...");
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
}

float webValue = 0;

void loop() {
  if (WiFiMulti.run() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure(); // Ignore SSL certificate

    HTTPClient https;

    // Manda nível do sensor pro Site
    if (nanoSerial.available() > 0) {
      delay(50);
      String inputData = nanoSerial.readStringUntil('\n'); // Read data until newline
      //delay(50);
      float currentValue = inputData.toFloat();
      Serial.print("nivel mandando pro site: ");
      Serial.println(currentValue);

      if (currentValue != lastValueSent && currentValue > 0) {
        //delay(50);
        String urlData = "https://controletanque.vercel.app/pages/api/levelToSite.js?level=" + String(currentValue);
        Serial.println(urlData);
        //delay(100);
        if (https.begin(*client, urlData)) {  // HTTPS
          //delay(100);
          int httpCode = https.GET();
          //delay(50);
          if (httpCode > 0) {
            Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              String payload = https.getString();
              Serial.println(payload);
            }
          } 

          // ------------------elses
          else {
            Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
          }
          https.end();
        } else {
          Serial.println("[HTTPS] Unable to connect");
        }
        
        lastValueSent = currentValue;
      }
    }

    // manda serial monitor como nivel pro site (testes)
    if (Serial.available() > 0) {
      delay(100);
      String inputData = Serial.readStringUntil('\n'); // Read data until newline
      delay(50);
      float currentValue = inputData.toFloat();
      Serial.print("nivel mandando pro site: ");
      Serial.println(currentValue);

      if (currentValue != lastValueSent && currentValue > 0) {
        delay(50);
        String urlData = "https://controletanque.vercel.app/pages/api/levelToSite.js?level=" + inputData;
        Serial.println(urlData);
        delay(100);
        if (https.begin(*client, urlData)) {  // HTTPS
          delay(100);
          int httpCode = https.GET();
          delay(50);
          if (httpCode > 0) {
            Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              String payload = https.getString();
              Serial.println(payload);
            }
          } 

          // ------------------elses
          else {
            Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
          }
          https.end();
        } else {
          Serial.println("[HTTPS] Unable to connect");
        }
        
        lastValueSent = currentValue;
      }
    }

    // Get data from the web and send it to serial monitor
    // Mandar requests pro NANO
    if (https.begin(*client, "https://controletanque.vercel.app/pages/api/levelFromSite.js")) {  // HTTPS
      int httpCode = https.GET();
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          webValue = payload.toFloat();

          if (webValue != lastValueReceived) {
            nanoSerial.println(webValue);
            lastValueReceived = webValue;
            Serial.print("sent to nanoSerial: ");
            Serial.println(webValue);
          }
        }
      } 

      // ------------------------------elses
      else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.println("[HTTPS] Unable to connect");
    }
  }
  else {
    Serial.println("WiFi not connected");
    delay(1000); // Wait for 1s before retrying
  }
  delay(100);
}
