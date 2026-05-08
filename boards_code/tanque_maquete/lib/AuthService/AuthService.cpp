#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "HardwareMap.h"
#include "AuthService.h"
#include "RFIDReader.h" // Agora depende apenas da nossa abstração

AuthService::AuthService() : _authorized(false), _activeUserID(""), _activeUserName("") {}

void AuthService::init() {
    // Inicializa o driver físico (seja ele PN532 ou RC522)
    rfidReader.init(); 
    Serial.println("[Auth] Serviço de Autenticação Online.");
}

bool AuthService::update() {
    // Se já estiver autorizado, não precisa validar novamente
    if (_authorized) return true;

    // Pergunta ao HAL se existe uma tag presente
    String uid = rfidReader.readTag();

    // Se o UID estiver vazio, não há nada para validar
    if (uid == "") return false;

    Serial.println("[Auth] Tag detectada: " + uid + ". Validando na Vercel...");

    // Inicia a validação via API se houver conexão WiFi
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("https://controle-tanque.vercel.app/api/auth/auth"); // URL ajustada conforme repositório[cite: 1]
        http.addHeader("Content-Type", "application/json");

        // Prepara o JSON para a API (ArduinoJson V7)
        JsonDocument doc; 
        doc["tag_id"] = uid;
        String jsonBody;
        serializeJson(doc, jsonBody);

        int httpCode = http.POST(jsonBody);

        if (httpCode == 200) {
            String response = http.getString();
            JsonDocument resDoc;
            deserializeJson(resDoc, response);
            
            _authorized = true;
            _activeUserID = uid;
            _activeUserName = resDoc["nome"].as<String>();
            
            Serial.printf("[Auth] Bem-vindo: %s\n", _activeUserName.c_str());
        } else {
            Serial.println("[Auth] Acesso negado. Código HTTP: " + String(httpCode));
        }
        http.end();
    } else {
        Serial.println("[Auth] Erro: Sem conexão WiFi para validar tag.");
    }

    return _authorized;
}

void AuthService::logout() {
    _authorized = false;
    _activeUserID = "";
    _activeUserName = "";
    Serial.println("[Auth] Sessão encerrada.");
}

bool AuthService::isAuthorized() { return _authorized; }
String AuthService::getActiveUserID() { return _activeUserID; }
String AuthService::getActiveUserName() { return _activeUserName; }

AuthService auth;