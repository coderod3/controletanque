#include "Auditoria.h"
#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

AuditoriaAPI Auditoria;
QueueHandle_t filaLogs;

void AuditoriaAPI::iniciar() {
    // Cria uma fila para até 15 operações (mais do que suficiente)
    filaLogs = xQueueCreate(15, sizeof(LogOperacao));
    sincronizarNTP();
    
    // Inicia a tarefa no Core 0 (junto com a rede, sem travar o Core 1 das bombas)
    xTaskCreatePinnedToCore(this->taskHTTP, "TaskHTTP", 8192, NULL, 1, NULL, 0);
}

void AuditoriaAPI::sincronizarNTP() {
    // Sincroniza em UTC 0. O fuso horário será tratado no Banco de Dados
    configTime(0, 0, NTP_SERVER, "time.nist.gov"); 
}

void AuditoriaAPI::registrar(LogOperacao log) {
    if (filaLogs != NULL) {
        xQueueSend(filaLogs, &log, (TickType_t)10);
    }
}

void AuditoriaAPI::taskHTTP(void *pvParameters) {
    LogOperacao log;
    
    while (true) {
        // Só tenta processar a fila se o Wi-Fi estiver OK E o relógio já sincronizou (Epoch > Ano 2023)
        if (WiFi.status() == WL_CONNECTED && time(nullptr) > 1672531200) {
            
            // xQueuePeek: Olha o próximo item sem tirá-lo da fila!
            if (xQueuePeek(filaLogs, &log, portMAX_DELAY) == pdPASS) {
                
                unsigned long agora_ms = millis();
                time_t epoch_atual = time(nullptr);

                // Proteção contra overflow: (agora_ms - evento_ms) é sempre seguro no ESP32
                time_t ts_fim = epoch_atual - ((agora_ms - log.fim_ms) / 1000);
                time_t ts_inicio = epoch_atual - ((agora_ms - log.inicio_ms) / 1000);
                time_t ts_recebido = epoch_atual - ((agora_ms - log.recebido_ms) / 1000);
                unsigned long duracao_ms = log.fim_ms - log.inicio_ms;

                // Montagem eficiente do JSON
                String json = "{";
                json += "\"tanque_id\":\"" + String(MQTT_CLIENT_ID) + "\",";
                json += "\"usuario_id\":\"" + String(log.usuario_id) + "\",";
                json += "\"tipo_operacao\":\"" + String(log.tipo_operacao) + "\",";
                json += "\"origem_comando\":\"" + String(log.origem_comando) + "\",";
                json += "\"status\":\"" + String(log.status) + "\",";
                json += "\"fisica\":{";
                json += "\"volume_alvo\":" + String(log.volume_alvo, 1) + ",";
                json += "\"volume_inicial\":" + String(log.volume_inicial, 1) + ",";
                json += "\"volume_final\":" + String(log.volume_final, 1) + ",";
                json += "\"duracao_ms\":" + String(duracao_ms);
                json += "},\"timestamps\":{";
                json += "\"recebido_placa\":" + String(ts_recebido) + ",";
                json += "\"inicio_execucao\":" + String(ts_inicio) + ",";
                json += "\"fim_execucao\":" + String(ts_fim);
                json += "}}";

                HTTPClient http;
                http.begin(API_AUDITORIA_URL);
                http.addHeader("Content-Type", "application/json");
                
                int httpCode = http.POST(json);
                
                if (httpCode == 200 || httpCode == 201) {
                    Serial.println("[AUDITORIA] Log salvo no servidor com sucesso.");
                    // SUCESSO! Agora sim, removemos da memória RAM
                    xQueueReceive(filaLogs, &log, 0); 
                } else {
                    Serial.printf("[AUDITORIA] Falha HTTP: %d. Tentando novamente mais tarde...\n", httpCode);
                    // Como não removemos com Receive, ele tentará novamente após o delay
                    vTaskDelay(5000 / portTICK_PERIOD_MS); 
                }
                http.end();
            }
        }
        
        // Espera 2 segundos antes de checar a fila/rede novamente
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}