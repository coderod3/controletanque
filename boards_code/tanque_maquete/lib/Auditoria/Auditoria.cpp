#include "Auditoria.h"
#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

AuditoriaAPI Auditoria;
QueueHandle_t filaLogs;

void AuditoriaAPI::iniciar() {
    filaLogs = xQueueCreate(15, sizeof(LogOperacao));
    
    xTaskCreatePinnedToCore(
        [](void* param) {
            AuditoriaAPI* instancia = (AuditoriaAPI*)param;
            instancia->taskHTTP(NULL);
        }, 
        "TaskHTTP", 
        16384, // Memória alta mantida para o HTTPS
        this, 
        1, 
        NULL, 
        0
    );
}

void AuditoriaAPI::sincronizarNTP() {
    Serial.println("[AUDITORIA-DEBUG] Sincronizando relogio NTP...");
    configTime(0, 0, NTP_SERVER, "time.nist.gov"); 
}

void AuditoriaAPI::registrar(LogOperacao log) {
    if (filaLogs != NULL) {
        if (xQueueSend(filaLogs, &log, (TickType_t)10) == pdPASS) {
            Serial.println("[AUDITORIA-DEBUG] Log enfileirado com sucesso. Aguardando envio.");
        } else {
            Serial.println("[AUDITORIA-DEBUG] ALERTA: Fila de logs CHEIA!");
        }
    }
}

void AuditoriaAPI::taskHTTP(void *pvParameters) {
    LogOperacao log;
    bool ntpSincronizado = false;
    
    while (true) {
        if (WiFi.status() == WL_CONNECTED) {
            
            if (!ntpSincronizado) {
                sincronizarNTP();
                ntpSincronizado = true;
                vTaskDelay(2000 / portTICK_PERIOD_MS); 
            }

            if (time(nullptr) > 1672531200) {
                
                if (xQueuePeek(filaLogs, &log, portMAX_DELAY) == pdPASS) {
                    
                    Serial.println("\n[AUDITORIA-DEBUG] --- INICIANDO ENVIO DE LOG ---");
                    unsigned long t_inicio_envio = millis();

                    unsigned long agora_ms = millis();
                    time_t epoch_atual = time(nullptr);

                    time_t ts_fim = epoch_atual - ((agora_ms - log.fim_ms) / 1000);
                    time_t ts_inicio = epoch_atual - ((agora_ms - log.inicio_ms) / 1000);
                    time_t ts_recebido = epoch_atual - ((agora_ms - log.recebido_ms) / 1000);
                    unsigned long duracao_ms = log.fim_ms - log.inicio_ms;

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

                    Serial.println("[AUDITORIA-DEBUG] Preparando Cliente Seguro HTTPS...");
                    WiFiClientSecure secureClient;
                    secureClient.setInsecure(); 

                    HTTPClient http;
                    http.begin(secureClient, API_AUDITORIA_URL);
                    http.addHeader("Content-Type", "application/json");
                    
                    Serial.println("[AUDITORIA-DEBUG] Disparando POST para a Vercel...");
                    unsigned long t_post = millis();
                    
                    disableCore0WDT();
                    int httpCode = http.POST(json);
                    enableCore0WDT(); 
                    
                    unsigned long t_gasto = millis() - t_post;
                    Serial.printf("[AUDITORIA-DEBUG] Servidor respondeu em: %lu milissegundos.\n", t_gasto);
                    
                    if (httpCode == 200 || httpCode == 201) {
                        Serial.println("[AUDITORIA-DEBUG] Sucesso! Log consumido da memoria.");
                        xQueueReceive(filaLogs, &log, 0); 
                    } else {
                        Serial.printf("[AUDITORIA-DEBUG] ERRO HTTP %d. Retentativa em 5s.\n", httpCode);
                        vTaskDelay(5000 / portTICK_PERIOD_MS); 
                    }
                    http.end();
                    Serial.println("[AUDITORIA-DEBUG] --- FIM DO ENVIO ---\n");
                }
            }
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}