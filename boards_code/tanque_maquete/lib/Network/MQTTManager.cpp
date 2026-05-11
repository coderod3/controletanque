#include "MQTTManager.h"
#include "WiFiService.h" // Necessário para a fila rxQueue e IncomingCommand
#include <ArduinoJson.h>

// Instâncias locais e privadas do Wi-Fi Seguro e do MQTT
static WiFiClientSecure espClient;
static PubSubClient mqttClient(espClient);

void MQTTManager::init() {
    espClient.setInsecure(); // Ignora certificados SSL complexos por velocidade
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(_mqttCallback);
    Serial.println("[MQTT] Servico inicializado.");
}

bool MQTTManager::isConnected() {
    return mqttClient.connected();
}

void MQTTManager::handle() {
    // Se caiu a conexão, tenta reconectar sem travar (sem while loop)
    if (!mqttClient.connected()) {
        Serial.print("[MQTT] Conectando ao Broker... ");
        
        // --- CONFIGURAÇÃO DE LAST WILL E TESTAMENT (LWT) ---
        const char* lwtTopic = "tanque/telemetria";
        const char* lwtMessage = "{\"status_operacional\": \"OFFLINE ❌\", \"operador\": \"Desconectado\"}";
        
        // Parametros: ID, User, Pass, Topico_LWT, QoS, Retain, Mensagem_LWT
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, lwtTopic, 0, true, lwtMessage)) {
            Serial.println("OK! Conectado.");
            mqttClient.subscribe("tanque/comando"); // Assina comandos do Dashboard
            
            // --- Publica ONLINE instantaneamente ao ligar ---
            String payloadOnline = "{\"status_operacional\": \"ONLINE ✅\"}";
            mqttClient.publish("tanque/telemetria", payloadOnline.c_str(), true);
        } else {
            Serial.print("Falha, rc=");
            Serial.println(mqttClient.state());
            // A task do Core 0 cuida do delay, retornamos para não engarrafar
            return; 
        }
    } else {
        // Mantém a escuta do broker ativa
        mqttClient.loop();
    }
}

void MQTTManager::publishTelemetria(float nivel, const char* operador) {
    if (!mqttClient.connected()) return;
    
    JsonDocument doc;
    doc["nivel"] = nivel;
    doc["status_operacional"] = operador; // Alinhado com a chave do Dashboard
    doc["timestamp"] = millis();
    
    char buffer[128];
    serializeJson(doc, buffer);
    mqttClient.publish("tanque/telemetria", buffer);
}

void MQTTManager::publishLog(const char* message, bool isError) {
    if (!mqttClient.connected()) return;
    mqttClient.publish("tanque/logs", message);
}

// Callback: Quando chega um comando da nuvem
void MQTTManager::_mqttCallback(char* topic, byte* payload, unsigned int length) {
    IncomingCommand cmd;
    memset(cmd.payload, 0, sizeof(cmd.payload));

    // Copia segura
    unsigned int copyLength = (length < sizeof(cmd.payload) - 1) ? length : sizeof(cmd.payload) - 1;
    for (unsigned int i = 0; i < copyLength; i++) {
        cmd.payload[i] = (char)payload[i];
    }

    // Joga na fila de entrada (O Core 1 vai ler isso instantaneamente)
    if (rxQueue != NULL) {
        xQueueSend(rxQueue, &cmd, 0);
    }
}