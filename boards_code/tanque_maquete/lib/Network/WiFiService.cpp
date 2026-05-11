#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "WiFiService.h"
#include "OTAManager.h"
#include "MQTTManager.h"
#include "CloudSync.h"

// Declaração das filas globais do FreeRTOS
QueueHandle_t txQueue = NULL;
QueueHandle_t rxQueue = NULL;
QueueHandle_t authRxQueue = NULL; // <-- ADICIONE ESTA LINHA

WiFiService::WiFiService() : _connected(false) {}

void WiFiService::init() {
    // 1. Criação das Filas
    txQueue = xQueueCreate(20, sizeof(NetworkEvent));     
    rxQueue = xQueueCreate(5, sizeof(IncomingCommand));   
    // Logo abaixo de onde você cria a rxQueue e txQueue, adicione:
    authRxQueue = xQueueCreate(2, sizeof(AuthResponseEvent));

    if (txQueue == NULL || rxQueue == NULL) {
        Serial.println("[CRITICAL] Erro de alocacao de memoria para as Filas!");
        return;
    }

    // 2. Disparo da Task de Rede no Core 0
    xTaskCreatePinnedToCore(
        this->_networkTask,
        "WiFiTask",
        12000, 
        this,
        1,
        NULL,
        0 // NÚCLEO ZERO
    );
}

bool WiFiService::isConnected() {
    return _connected;
}

// =============================================================================
// FUNÇÕES DE EMPACOTAMENTO (CHAMADAS PELO CORE 1) - NÃO BLOQUEANTES
// =============================================================================

bool WiFiService::queueTelemetria(float nivel, String operador) {
    NetworkEvent event;
    event.type = EVENT_MQTT_TELEMETRIA;
    event.level = nivel;
    memset(event.status, 0, sizeof(event.status));
    strncpy(event.status, operador.c_str(), sizeof(event.status) - 1);
    
    return (xQueueSend(txQueue, &event, 0) == pdPASS);
}

bool WiFiService::queueDigitalTwin(String status, float nivel) {
    NetworkEvent event;
    event.type = EVENT_HTTP_DIGITAL_TWIN;
    event.level = nivel;
    memset(event.status, 0, sizeof(event.status));
    strncpy(event.status, status.c_str(), sizeof(event.status) - 1);
    
    return (xQueueSend(txQueue, &event, 0) == pdPASS);
}

bool WiFiService::queueAuthRequest(String rfid) {
    NetworkEvent event;
    event.type = EVENT_HTTP_AUTH_REQUEST;
    memset(event.rfid_uid, 0, sizeof(event.rfid_uid));
    strncpy(event.rfid_uid, rfid.c_str(), sizeof(event.rfid_uid) - 1);
    
    return (xQueueSend(txQueue, &event, 0) == pdPASS);
}

bool WiFiService::readAuthResponse(bool& isAuthorized, String& userName) {
    AuthResponseEvent response;
    // Se tiver resposta na fila, copia os dados e retorna true
    if (xQueueReceive(authRxQueue, &response, 0) == pdPASS) {
        isAuthorized = response.isAuthorized;
        userName = String(response.userName);
        return true; 
    }
    return false; // Se a Vercel ainda não respondeu, retorna false instantaneamente
}

bool WiFiService::queueAuditLog(String rfid, String acao, float volume, float anterior, float atual) {
    NetworkEvent event;
    event.type = EVENT_HTTP_AUDITORIA;
    event.level = volume;
    memset(event.rfid_uid, 0, sizeof(event.rfid_uid));
    memset(event.acao, 0, sizeof(event.acao));
    strncpy(event.rfid_uid, rfid.c_str(), sizeof(event.rfid_uid) - 1);
    strncpy(event.acao, acao.c_str(), sizeof(event.acao) - 1);
    event.valor_anterior = anterior;
    event.valor_atual = atual;
    
    return (xQueueSend(txQueue, &event, 0) == pdPASS);
}

bool WiFiService::queueLog(String message, bool isError) {
    NetworkEvent event;
    event.type = isError ? EVENT_MQTT_LOG_ERROR : EVENT_MQTT_LOG_INFO;
    memset(event.log_message, 0, sizeof(event.log_message));
    strncpy(event.log_message, message.c_str(), sizeof(event.log_message) - 1);
    
    return (xQueueSend(txQueue, &event, 0) == pdPASS);
}

bool WiFiService::readPendingCommand(IncomingCommand& outCommand) {
    return (xQueueReceive(rxQueue, &outCommand, 0) == pdPASS);
}

// =============================================================================
// TAREFA DE FUNDO (CORE 0) - ORQUESTRADOR
// =============================================================================
void WiFiService::_networkTask(void* pvParameters) {
    WiFiService* instance = (WiFiService*)pvParameters;
    
    Serial.println("[Comm] Orquestrador de Rede iniciado no Core 0");

    WiFi.mode(WIFI_STA);

    // --- CONFIGURAÇÃO DE IP ESTÁTICO ---
    IPAddress local_IP(192, 168, 0, 115);
    IPAddress gateway(192, 168, 0, 1);      
    IPAddress subnet(255, 255, 255, 0);
    IPAddress primaryDNS(8, 8, 8, 8);       
    IPAddress secondaryDNS(8, 8, 4, 4);

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("[WiFi] Erro ao configurar IP Estático!");
    } else {
        Serial.println("[WiFi] IP Estático definido para: 192.168.0.115");
    }
    // -----------------------------------

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    MQTTManager::init();

    bool otaSetupDone = false;
    static bool wasConnected = false; // FASE 3: Detetor de transição de rede
    NetworkEvent pendingEvent;

    for (;;) {
        // --- 1. Manutenção de Conexão (Wi-Fi, OTA e MQTT) ---
        if (WiFi.status() == WL_CONNECTED) {
            
            // FASE 3: ACABOU DE CONECTAR! DISPARA O FLUSH DOS LOGS OFFLINE
            if (!wasConnected) {
                wasConnected = true;
                instance->_connected = true;
                
                Serial.println("\n====================================");
                Serial.println("[WiFi] CONECTADO COM SUCESSO!");
                Serial.print("[WiFi] IP ATUAL: ");
                Serial.println(WiFi.localIP());
                Serial.println("====================================\n");
                
                if (!otaSetupDone) {
                    OTAManager::init("nexus-tank-esp32");
                    otaSetupDone = true;
                }

                // Descarrega os logs que a placa guardou enquanto estava sem rede
                // Faz isso antes de aceitar comandos novos
                cloud.flushOfflineLogs();
            }

            if (otaSetupDone) OTAManager::handle();
            MQTTManager::handle();

        } else {
            // A rede caiu.
            wasConnected = false;
            instance->_connected = false;
            
            static unsigned long lastReconnect = 0;
            if (millis() - lastReconnect > 10000) {
                lastReconnect = millis();
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            }
        }

        // --- 2. Processamento da Fila de Transmissão (Despacho) ---
        if (xQueueReceive(txQueue, &pendingEvent, pdMS_TO_TICKS(5)) == pdPASS) {
            
            // ATENÇÃO: Auditoria agora é chamada MESMO SE OFFLINE! 
            // O próprio `sendAuditLog` do CloudSync vai decidir se guarda fisicamente ou envia.
            if (pendingEvent.type == EVENT_HTTP_AUDITORIA) {
                cloud.sendAuditLog(pendingEvent.rfid_uid, pendingEvent.acao, 
                                   pendingEvent.level, pendingEvent.valor_anterior, 
                                   pendingEvent.valor_atual);
            }
            
            // As restantes operações dependem de rede ao vivo
            else if (WiFi.status() == WL_CONNECTED) {
                switch (pendingEvent.type) {
                    case EVENT_MQTT_TELEMETRIA:
                        MQTTManager::publishTelemetria(pendingEvent.level, pendingEvent.status);
                        break;

                    case EVENT_HTTP_DIGITAL_TWIN:
                        cloud.syncDigitalTwin(pendingEvent.status, pendingEvent.level);
                        break;

                    case EVENT_HTTP_AUTH_REQUEST: {
                        // Delega a consulta à nuvem para o CloudSync, deixando o código do Wi-Fi limpo
                        String nome;
                        bool isAuth = cloud.authenticateTag(pendingEvent.rfid_uid, nome);
                        
                        AuthResponseEvent authRes;
                        authRes.isAuthorized = isAuth;
                        memset(authRes.userName, 0, sizeof(authRes.userName));
                        strncpy(authRes.userName, nome.c_str(), sizeof(authRes.userName) - 1);
                        xQueueSend(authRxQueue, &authRes, 0);
                        break;
                    }

                    case EVENT_MQTT_LOG_INFO:
                        MQTTManager::publishLog(pendingEvent.log_message, false);
                        break;

                    case EVENT_MQTT_LOG_ERROR:
                        MQTTManager::publishLog(pendingEvent.log_message, true);
                        break;
                }
            }
        }

        // Delay crítico para o RTOS respirar e não disparar o Watchdog Timer
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

WiFiService connectivity;