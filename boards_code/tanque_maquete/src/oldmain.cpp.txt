#include <Arduino.h>
#include <ArduinoJson.h>
#include "HardwareMap.h"
#include "Config.h"

// Inclusão dos módulos isolados (agora na pasta lib/)
#include "TankPhysics.h"
#include "DisplayManager.h"
#include "WiFiService.h"
#include "AuthService.h"
#include "InputManager.h"
#include "CloudSync.h"
#include "TankController.h"

// Protótipo da função de comunicação
void handleCommunication();

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    
    // Inicialização modular de todos os serviços
    tank.init();
    display.init();
    connectivity.init(); 
    auth.init();
    inputs.init();
    controller.init(); // Inicializa FSM, Bombas e Volume Virtual

    // Configuração de interrupção de hardware (Transbordamento)
    // Usamos uma função lambda para chamar o método de emergência do controller
    pinMode(PIN_OVERFLOW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_OVERFLOW), []() {
        controller.emergencyStop();
    }, FALLING);

    display.showStatus("NEXUS OS", "SISTEMA ONLINE");
}

// =============================================================================
// LOOP PRINCIPAL
// =============================================================================
void loop() {
    // 1. Atualização dos Sensores Físicos
    tank.update();

    // 2. Atualização das Entradas do Usuário (Botões e RFID)
    inputs.update();

    // 3. Gerenciamento de Comunicação MQTT e Telemetria
    handleCommunication();

    // 4. Processamento da Inteligência e Controle (Máquina de Estados)
    controller.update();
}

// =============================================================================
// ROTEAMENTO DE COMUNICAÇÃO (MQTT -> CONTROLLER)
// =============================================================================
void handleCommunication() {
    // 1. Publicação de Telemetria (A cada 2 segundos)
    static unsigned long lastPub = 0;
    if (millis() - lastPub > 2000) { 
        connectivity.publishTelemetria(tank.getVolume(), auth.getActiveUserName());
        lastPub = millis();
    }

    // 2. Processamento de comandos recebidos via MQTT
    String jsonRaw = connectivity.getPendingCommand();
    if (jsonRaw != "") {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, jsonRaw);
        
        if (error) {
            connectivity.queueLog("ERRO_JSON_REMOTE");
            return;
        }

        String acao = doc["acao"] | "";

        // Roteamento de comandos para os métodos públicos do TankController
        if (acao == "PARAR" || acao == "DESLIGAR") {
            controller.emergencyStop();
            connectivity.queueLog("STOP_REMOTO_EXECUTADO");
        } 
        else if (acao == "EXECUTAR") {
            float vol = doc["volume"] | 0.0;
            bool encher = doc["encher"] | true;
            controller.addJob(vol, encher, "REMOTO");
        }
        else if (acao == "SYNC_CONFIG") {
            // Sincroniza calibração no módulo de Física
            tank.syncConfig(
                doc["max_volume"] | tank.getMaxVolume(), 
                doc["dist_vazio"] | tank.getRawDistance(), 
                doc["dist_cheio"] | 1.0
            );
            
            // Força o controlador a reajustar o volume virtual
            controller.forceSyncVirtual();
            
            connectivity.queueLog("CONFIG_SYNC_OK");
            display.showStatus("CALIBRADO", String(tank.getMaxVolume()) + "L OK");
        }
    }
}