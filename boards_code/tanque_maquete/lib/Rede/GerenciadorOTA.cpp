#include "GerenciadorOTA.h"
#include <ArduinoOTA.h>
#include "HardwareMap.h"

GerenciadorOTAAPI GerenciadorOTA;

void GerenciadorOTAAPI::iniciar() {
    ArduinoOTA.setPort(3232);
    ArduinoOTA.setHostname("nexus-tank-esp32");

    ArduinoOTA.onStart([]() {
        Serial.println("[OTA] Iniciando atualizacao. Desligando bombas...");
        // Trava de segurança física (Core 0 agindo preventivamente)
        digitalWrite(PIN_BOMBA_ENCHER, LOW);
        digitalWrite(PIN_BOMBA_ESVAZ, LOW);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Sucesso! Reiniciando...");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progresso: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Erro [%u]\n", error);
    });

    ArduinoOTA.begin();
    Serial.println("[OTA] Servico iniciado.");
}

void GerenciadorOTAAPI::lidar() {
    ArduinoOTA.handle();
}