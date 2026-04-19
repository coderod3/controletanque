#include <Arduino.h>
#include "Config.h"
#include "WiFiService.h"

WiFiService::WiFiService() : _connected(false) {}

void WiFiService::init() {
    // Criamos a tarefa no Core 0. Prioridade 1 (baixa) para não atropelar o sistema.
    xTaskCreatePinnedToCore(
        this->_networkTask,   // Função da tarefa
        "WiFiTask",           // Nome
        8192,                 // Stack size (WiFi consome bastante)
        this,                 // Parâmetro (passamos a instância da classe)
        1,                    // Prioridade
        NULL,                 // Task handle
        0                     // Core 0 (O rádio do ESP32 prefere o Core 0)
    );
}

bool WiFiService::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void WiFiService::_networkTask(void* pvParameters) {
    WiFiService* instance = (WiFiService*)pvParameters;
    
    Serial.println("[WiFi] Tarefa iniciada no Core 0");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    for (;;) { // Loop infinito da tarefa
        if (WiFi.status() == WL_CONNECTED) {
            if (!instance->_connected) {
                Serial.println("[WiFi] Conectado!");
                instance->_connected = true;
                // Aqui seria o lugar de descarregar os logs acumulados via MQTT/HTTP
            }
        } else {
            if (instance->_connected) {
                Serial.println("[WiFi] Conexão perdida. Tentando reconectar...");
                instance->_connected = false;
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            }
        }

        // Importante: vTaskDelay permite que o RTOS gerencie outras tarefas do sistema
        // Ao contrário de delay(), isso não trava o núcleo.
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}

void WiFiService::queueLog(String message) {
    // Implementação básica de fila para logs offline
    _logBuffer[_head] = message;
    _head = (_head + 1) % LOG_BUFFER_SIZE;
}

WiFiService connectivity;