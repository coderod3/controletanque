#include "CloudSync.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h> 
#include <ArduinoJson.h>

CloudSync::CloudSync() : _baseUrl("https://controletanque.vercel.app/api") {}

void CloudSync::syncDigitalTwin(String status, float nivel) {
    if (WiFi.status() != WL_CONNECTED) return; // Digital twin é em tempo real, se falhar não precisa reter

    WiFiClientSecure secureClient;
    secureClient.setInsecure(); 

    HTTPClient http;
    http.begin(secureClient, _baseUrl + "/telemetria/status");
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["status"] = status;
    doc["nivel"] = nivel;

    String json;
    serializeJson(doc, json);
    http.POST(json);
    http.end();
}

// FASE 3: Envio Seguro (Guarda offline se falhar)
void CloudSync::sendAuditLog(String rfid_uid, String acao, float volume, float anterior, float atual) {
    JsonDocument doc;
    doc["rfid_uid"] = rfid_uid;
    doc["acao"] = acao;
    doc["volume"] = volume;
    doc["valor_anterior"] = anterior;
    doc["valor_atual"] = atual;

    String json;
    serializeJson(doc, json);

    // Sem rede? Guarda fisicamente e aborta.
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Cloud] Sem Wi-Fi. Guardando log offline...");
        saveLogOffline(json);
        return;
    }

    WiFiClientSecure secureClient;
    secureClient.setInsecure(); 

    HTTPClient http;
    http.begin(secureClient, _baseUrl + "/telemetria/auditoria");
    http.addHeader("Content-Type", "application/json");

    int code = http.POST(json);
    http.end();
    
    if (code == 200) {
        Serial.println("[Cloud] Auditoria enviada com sucesso.");
    } else {
        Serial.printf("[Cloud] Falha na Vercel (HTTP %d). Guardando log offline...\n", code);
        saveLogOffline(json); // A rede existe, mas o servidor caiu. Guarda na mesma!
    }
}

// FASE 3: Lógica do Ring Buffer NVS
void CloudSync::saveLogOffline(String jsonPayload) {
    _prefs.begin("offline_logs", false); // Abre modo leitura/escrita
    
    int count = _prefs.getInt("count", 0);
    if (count < 50) { // Limite de segurança para não corromper a Flash
        String key = "log_" + String(count);
        _prefs.putString(key.c_str(), jsonPayload);
        _prefs.putInt("count", count + 1);
        Serial.println("[Cloud] Retido com sucesso. Fila pendente: " + String(count + 1));
    } else {
        Serial.println("[Cloud] ERRO CRITICO: Fila offline cheia (50/50). Log descartado.");
    }
    
    _prefs.end();
}

// FASE 3: Despejo Automático Assíncrono (Pega o Índice 0 e recua a fila)
bool CloudSync::flushNextOfflineLog() {
    if (WiFi.status() != WL_CONNECTED) return false;

    _prefs.begin("offline_logs", false);
    int count = _prefs.getInt("count", 0);
    
    if (count == 0) {
        _prefs.end();
        return false; // Fila vazia
    }

    // Lê sempre o mais antigo (índice 0)
    String json = _prefs.getString("log_0", "");
    if (json == "") {
        _prefs.end();
        return false; 
    }

    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    HTTPClient http;
    http.begin(secureClient, _baseUrl + "/telemetria/auditoria");
    http.addHeader("Content-Type", "application/json");
    
    int code = http.POST(json);
    http.end();

    if (code == 200) {
        // Sucesso: Apaga o enviado e empurra a fila inteira para frente
        for (int i = 1; i < count; i++) {
            String oldKey = "log_" + String(i);
            String newKey = "log_" + String(i - 1);
            _prefs.putString(newKey.c_str(), _prefs.getString(oldKey.c_str(), ""));
        }
        // Apaga o último índice que ficou duplicado no final
        _prefs.remove(("log_" + String(count - 1)).c_str());
        _prefs.putInt("count", count - 1);
        
        Serial.printf("[Cloud] Log offline entregue. Restam: %d\n", count - 1);
        _prefs.end();
        return true; // True = "Tenho mais trabalho, chame-me de novo"
    } else {
        Serial.printf("[Cloud] Erro no Server (HTTP %d). Pausando fila...\n", code);
        _prefs.end();
        return false; // False = "Erro, pare de chamar por agora"
    }
}

bool CloudSync::authenticateTag(String rfid_uid, String& outName) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.begin(_baseUrl + "/auth/auth");
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