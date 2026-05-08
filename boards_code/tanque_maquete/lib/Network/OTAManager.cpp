#include "OTAManager.h"
#include "HardwareMap.h" // Para acessar as travas das bombas no OTA

void OTAManager::init(const char* hostname) {
    ArduinoOTA.setPort(3232);
    ArduinoOTA.setHostname(hostname);

    ArduinoOTA.onStart([]() {
        Serial.println("\n[OTA] Atualizacao Iniciada. Parando atuadores por seguranca...");
        digitalWrite(PIN_BOMBA_ENCHER, LOW);
        digitalWrite(PIN_BOMBA_ESVAZ, LOW);
    });
    
    ArduinoOTA.onEnd([]() { Serial.println("\n[OTA] Sucesso! Reiniciando o ESP32."); });
    ArduinoOTA.onError([](ota_error_t error) { Serial.printf("\n[OTA] Erro de transmissao: %u", error); });
    
    ArduinoOTA.begin();
    Serial.println("[OTA] Servico de escuta iniciado com sucesso.");
}

void OTAManager::handle() {
    ArduinoOTA.handle();
}