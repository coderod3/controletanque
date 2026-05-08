#include "CloudSync.h"
#include <WiFi.h>

CloudSync::CloudSync() {}

void CloudSync::syncDigitalTwin(String status, float nivel) {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    // Corrigido para a URL sem hífen conforme seu deploy
    http.begin(_baseUrl + "/telemetria/status"); 
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc; // ArduinoJson V7
    doc["status"] = status;
    doc["nivel"] = nivel;

    String json;
    serializeJson(doc, json);
    http.POST(json);
    http.end();
}

void CloudSync::sendAuditLog(String rfid, String acao, float volume, float anterior, float atual) {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.begin(_baseUrl + "/telemetria/auditoria");
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc; // ArduinoJson V7
    doc["rfid_uid"] = rfid;
    doc["acao"] = acao;
    doc["volume"] = volume;
    doc["valor_anterior"] = anterior;
    doc["valor_atual"] = atual;
    doc["status"] = "SUCESSO";

    String json;
    serializeJson(doc, json);
    http.POST(json);
    http.end();
}

CloudSync cloud;