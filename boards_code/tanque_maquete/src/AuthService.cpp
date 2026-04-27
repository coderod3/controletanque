#include <Arduino.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "AuthService.h"

AuthService::AuthService() : _mfrc522(PIN_RFID_SS, PIN_RFID_RST), _authorized(false), _activeUserID("") {}

void AuthService::init() {
    SPI.begin(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);
    _mfrc522.PCD_Init();
    Serial.println("[Auth] RFID Online.");
}

bool AuthService::update() {
    if (_authorized) return true;

    // Detecta nova tag
    if (!_mfrc522.PICC_IsNewCardPresent() || !_mfrc522.PICC_ReadCardSerial()) return false;

    // Converte UID para String Hex
    String uid = "";
    for (byte i = 0; i < _mfrc522.uid.size; i++) {
        uid += String(_mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        uid += String(_mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    Serial.println("[Auth] Validando na nuvem: " + uid);

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        // Caminho exato que você criou
        http.begin("https://controletanque.vercel.app/api/auth/auth");
        http.addHeader("Content-Type", "application/json");

        // Prepara o JSON seguro
        StaticJsonDocument<128> doc;
        doc["tag_id"] = uid;
        String jsonBody;
        serializeJson(doc, jsonBody);

        int httpCode = http.POST(jsonBody);

        if (httpCode == 200) {
            String response = http.getString();
            StaticJsonDocument<200> resDoc;
            deserializeJson(resDoc, response);
            
            _authorized = true;
            _activeUserID = uid;
            _activeUserName = resDoc["nome"].as<String>(); // SALVA O NOME AQUI
            Serial.printf("[Auth] Bem-vindo: %s (%s)\n", resDoc["nome"].as<const char*>(), resDoc["cargo"].as<const char*>());
        } else {
            Serial.println("[Auth] Acesso negado. Código: " + String(httpCode));
            delay(1000); // Evita múltiplas tentativas imediatas
        }
        http.end();
    }

    _mfrc522.PICC_HaltA();
    _mfrc522.PCD_StopCrypto1();
    return _authorized;
}

void AuthService::logout() {
    _authorized = false;
    _activeUserID = "";
    Serial.println("[Auth] Logoff realizado.");
}

bool AuthService::isAuthorized() { return _authorized; }
String AuthService::getActiveUserID() { return _activeUserID; }

AuthService auth;