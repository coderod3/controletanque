#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "WiFiService.h"

// Objetos de rede globais (estáticos ao arquivo)
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Inicialização de membros estáticos da classe
String WiFiService::_lastCommand = "";
bool WiFiService::_hasNewCommand = false;

WiFiService::WiFiService() : _connected(false) {}

void WiFiService::init() {
    // Mantemos sua tarefa no Core 0. Aumentei o Stack para 10000 para lidar com SSL/JSON.
    xTaskCreatePinnedToCore(
        this->_networkTask,
        "WiFiTask",
        10000,
        this,
        1,
        NULL,
        0
    );
}

bool WiFiService::isConnected() {
    return (WiFi.status() == WL_CONNECTED && mqttClient.connected());
}

void WiFiService::publishTelemetria(float nivel, String operador) {
    if (mqttClient.connected()) {
        StaticJsonDocument<128> doc;
        doc["nivel"] = nivel;
        doc["operador"] = operador == "" ? "Nenhum" : operador; // Envia o nome ou "Nenhum"
        doc["timestamp"] = millis();
        
        char buffer[128];
        serializeJson(doc, buffer);
        mqttClient.publish(TOPIC_TELEMETRIA, buffer);
    }
}

void WiFiService::queueLog(String message) {
    // Sua lógica de buffer original
    _logBuffer[_head] = message;
    _head = (_head + 1) % LOG_BUFFER_SIZE;
    
    // Tentativa imediata de envio se estiver conectado
    if (mqttClient.connected()) {
        mqttClient.publish("tanque/logs", message.c_str());
    }
}

String WiFiService::getPendingCommand() {
    if (_hasNewCommand) {
        _hasNewCommand = false;
        return _lastCommand;
    }
    return "";
}

// =============================================================================
// TAREFA DE FUNDO (CORE 0)
// =============================================================================
void WiFiService::_networkTask(void* pvParameters) {
    WiFiService* instance = (WiFiService*)pvParameters;
    
    Serial.println("[Comm] Tarefa de Comunicação iniciada no Core 0");
    
    // Configura SSL para o HiveMQ (Ignora validação para facilitar o desenvolvimento)
    espClient.setInsecure();
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(_mqttCallback);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    for (;;) {
        // Gerenciamento de WiFi
        if (WiFi.status() == WL_CONNECTED) {
            if (!instance->_connected) {
                Serial.println("[WiFi] Conectado!");
                instance->_connected = true;
            }

            // Gerenciamento de MQTT
            if (!mqttClient.connected()) {
                Serial.print("[MQTT] Tentando conexão...");
                if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
                    Serial.println(" Sucesso!");
                    mqttClient.subscribe(TOPIC_COMANDO);
                } else {
                    Serial.print(" Falha, rc=");
                    Serial.println(mqttClient.state());
                    vTaskDelay(pdMS_TO_TICKS(5000));
                }
            } else {
                mqttClient.loop();
            }
        } else {
            instance->_connected = false;
            // Se perder WiFi, tenta reconectar periodicamente
            if (millis() % 10000 < 100) {
                 WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Delay menor para resposta rápida do MQTT
    }
}

// Procure esta linha dentro do _mqttCallback:
void WiFiService::_mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) message += (char)payload[i];
    
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {
        // MUDE ESTA LINHA:
        // _lastCommand = doc["acao"] | ""; 
        // PARA ESTA:
        _lastCommand = message; // Agora passamos o JSON bruto para o main
        
        _hasNewCommand = true;
    }
}

WiFiService connectivity;