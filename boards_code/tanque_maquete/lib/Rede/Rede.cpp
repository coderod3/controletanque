#include "Rede.h"
#include "ConexaoWiFi.h"
#include "GerenciadorMQTT.h"
#include "GerenciadorOTA.h"

QueueHandle_t filaTX = NULL;
QueueHandle_t filaRX = NULL;
RedeAPI Rede;

void RedeAPI::iniciar() {
    // 1. Cria as filas do FreeRTOS
    filaTX = xQueueCreate(20, sizeof(MensagemSaida));
    filaRX = xQueueCreate(10, sizeof(ComandoEntrada));

    // 2. Dispara o orquestrador no CORE 0 usando Lambda Segura
    xTaskCreatePinnedToCore(
        [](void* param) {
            RedeAPI* instancia = (RedeAPI*)param;
            instancia->_taskRede(NULL);
        },
        "TaskRede",
        10000, 
        this, // Injeta a própria classe no parâmetro do FreeRTOS
        1,
        NULL,
        0 // NÚCLEO ZERO
    );
}

void RedeAPI::_taskRede(void* pvParameters) {
    Serial.println("[Rede] Orquestrador iniciado no Core 0");

    // Inicializa os sub-módulos
    ConexaoWiFi.iniciar();
    GerenciadorMQTT.iniciar();
    GerenciadorOTA.iniciar();

    for (;;) {
        // 1. Mantém o link WiFi
        ConexaoWiFi.manter();

        if (ConexaoWiFi.estaConectado()) {
            // 2. Processa o OTA
            GerenciadorOTA.lidar();

            // 3. Processa o MQTT
            GerenciadorMQTT.processar();
        }

        // Delay para o RTOS não disparar o Watchdog
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool RedeAPI::estaOnline() {
    return ConexaoWiFi.estaConectado() && GerenciadorMQTT.conectado();
}

void RedeAPI::enviar(String topico, String json) {
    MensagemSaida msg;
    strncpy(msg.topico, topico.c_str(), 31);
    strncpy(msg.payload, json.c_str(), 127);
    xQueueSend(filaTX, &msg, 0);
}

bool RedeAPI::temComando(ComandoEntrada& out) {
    return (xQueueReceive(filaRX, &out, 0) == pdPASS);
}