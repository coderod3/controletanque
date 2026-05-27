#include "ConexaoWiFi.h"
#include <WiFi.h>
#include "Config.h"

ConexaoWiFiAPI ConexaoWiFi;

void ConexaoWiFiAPI::iniciar() {
    _conectado = false;
    _ultimaTentativa = 0;

    WiFi.mode(WIFI_STA);

    // Configuração de IP Estático conforme solicitado (.115)
    // Ajustado para a sub-rede 192.168.1.x (comum na maioria dos roteadores)
    IPAddress local_IP(192, 168, 0, 115);
    IPAddress gateway(192, 168, 0, 1);      
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns(8, 8, 8, 8);

    if (!WiFi.config(local_IP, gateway, subnet, dns)) {
        Serial.println("[WiFi] Erro ao configurar IP Estatico!");
    }

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
        // Tenta reconectar a cada 10 segundos se cair, sem travar o loop
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