#include "MQTTManager.h"
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
    if (!mqttClient.connected()) {
        Serial.print("[MQTT] Tentando conexao com o Broker... ");
        // Tenta conectar
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
            Serial.println("Conectado!");
            mqttClient.subscribe(TOPIC_COMANDO); // Assina o tópico de comandos
        } else {
            Serial.print("Falha, rc=");
            Serial.println(mqttClient.state());
        }
    } else {
        // Mantém a escuta do broker ativa
        mqttClient.loop();
    }
}

void MQTTManager::publishTelemetria(float nivel, const char* operador) {
    if (!mqttClient.connected()) return;
    
    StaticJsonDocument<128> doc;
    doc["nivel"] = nivel;
    doc["operador"] = operador;
    doc["timestamp"] = millis();
    
    char buffer[128];
    serializeJson(doc, buffer);
    mqttClient.publish(TOPIC_TELEMETRIA, buffer);
}

void MQTTManager::publishLog(const char* message, bool isError) {
    if (!mqttClient.connected()) return;
    
    // Se for erro, pode mandar para um tópico diferente no futuro, por enquanto vai no padrão
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