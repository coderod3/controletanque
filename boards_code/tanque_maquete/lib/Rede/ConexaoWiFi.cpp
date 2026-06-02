#include "ConexaoWiFi.h"
#include <WiFi.h>
#include "Config.h"

ConexaoWiFiAPI ConexaoWiFi;

void ConexaoWiFiAPI::iniciar() {
    _conectado = false;
    _ultimaTentativa = 0;

    WiFi.mode(WIFI_STA);
    
    // Força a desconexão de qualquer rede velha salva na memória
    WiFi.disconnect(true);
    delay(100);

    // Inicia 100% via DHCP (Funciona em Roteador ou Celular)
    Serial.print("[WiFi] Conectando a rede: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void ConexaoWiFiAPI::manter() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!_conectado) {
            _conectado = true;
            Serial.print("[WiFi] Conectado! IP: ");
            Serial.println(WiFi.localIP());
        }
    } else {
        _conectado = false;
        if (millis() - _ultimaTentativa > 10000) {
            _ultimaTentativa = millis();
            Serial.println("[WiFi] Tentando reconectar...");
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    }
}

bool ConexaoWiFiAPI::estaConectado() {
    return _conectado;
}