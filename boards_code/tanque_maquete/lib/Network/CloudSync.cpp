#include "CloudSync.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h> // <-- ADICIONE AQUI
#include <ArduinoJson.h>

CloudSync::CloudSync() : _baseUrl("https://controletanque.vercel.app/api") {}

void CloudSync::syncDigitalTwin(String status, float nivel) {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure secureClient;
    secureClient.setInsecure(); // <-- IGNORA O CERTIFICADO SSL

    HTTPClient http;
    http.begin(secureClient, _baseUrl + "/telemetria/status");
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["status"] = status;
    doc["nivel"] = nivel;

    String json;
    serializeJson(doc, json);
    int code = http.POST(json);
    
    if (code != 200) Serial.printf("[Cloud] Erro Digital Twin: %d\n", code);
    http.end();
}

void CloudSync::sendAuditLog(String rfid_uid, String acao, float volume, float anterior, float atual) {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure secureClient;
    secureClient.setInsecure(); // <-- IGNORA O CERTIFICADO SSL

    HTTPClient http;
    http.begin(secureClient, _baseUrl + "/telemetria/auditoria");
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["rfid_uid"] = rfid_uid;
    doc["acao"] = acao;
    doc["volume"] = volume;
    doc["valor_anterior"] = anterior;
    doc["valor_atual"] = atual;

    String json;
    serializeJson(doc, json);
    int code = http.POST(json);
    
    if (code != 200) Serial.printf("[Cloud] Erro de Auditoria: %d\n", code);
    http.end();
}

bool CloudSync::authenticateTag(String rfid_uid, String& outName) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.begin(_baseUrl + "/auth/auth"); // Bate na sua API
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["tag_id"] = rfid_uid;

    String json;
    serializeJson(doc, json);
    
    int httpCode = http.POST(json);
    bool authorized = false;

    if (httpCode == 200) {
        String payload = http.getString();
        JsonDocument resDoc;
        deserializeJson(resDoc, payload);
        
        if (resDoc["authorized"] == true) {
            authorized = true;
            outName = resDoc["nome"].as<String>();
        }
    }
    
    http.end();
    return authorized;
}

CloudSync cloud;