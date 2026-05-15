#include "GerenciadorMQTT.h"
#include "Rede.h"
#include "Config.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

static WiFiClientSecure espClient;
static PubSubClient mqttClient(espClient);
GerenciadorMQTTAPI GerenciadorMQTT;

void GerenciadorMQTTAPI::iniciar() {
    // Configuração de segurança para HiveMQ Cloud
    espClient.setInsecure(); 
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(this->_callback);
    Serial.println("[MQTT] Servico inicializado.");
}

bool GerenciadorMQTTAPI::conectado() {
    return mqttClient.connected();
}

void GerenciadorMQTTAPI::processar() {
    if (!mqttClient.connected()) {
        _tentarReconectar();
        return;
    }

    mqttClient.loop();

    // --- LOGÍSTICA DE SAÍDA (TX) ---
    // O MQTT retira da fila e envia para a nuvem
    MensagemSaida msg;
    if (xQueueReceive(filaTX, &msg, 0) == pdPASS) {
        mqttClient.publish(msg.topico, msg.payload);
        Serial.print("[MQTT] Enviado para: ");
        Serial.println(msg.topico);
    }
}

void GerenciadorMQTTAPI::_tentarReconectar() {
    static unsigned long ultimaTentativa = 0;
    if (millis() - ultimaTentativa < 5000) return;
    ultimaTentativa = millis();

    Serial.print("[MQTT] Conectando ao Broker... ");
    
    // Configuração de Last Will (LWT) inspirada no seu código antigo
    const char* lwtTopic = TOPIC_TELEMETRIA;
    const char* lwtMsg = "{\"status\": \"OFFLINE\"}";

    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, lwtTopic, 0, true, lwtMsg)) {
        Serial.println("OK!");
        mqttClient.subscribe(TOPIC_COMANDO);
    } else {
        Serial.print("Falha, rc=");
        Serial.println(mqttClient.state());
    }
}

// --- LOGÍSTICA DE ENTRADA (RX) ---
// Quando chega algo do site, o MQTT empacota e joga na fila para o Core 1
void GerenciadorMQTTAPI::_callback(char* topic, byte* payload, unsigned int length) {
    ComandoEntrada cmd;
    memset(cmd.payload, 0, sizeof(cmd.payload));

    // Copia segura dos dados brutos
    unsigned int len = (length < sizeof(cmd.payload) - 1) ? length : sizeof(cmd.payload) - 1;
    memcpy(cmd.payload, payload, len);

    if (filaRX != NULL) {
        xQueueSend(filaRX, &cmd, 0);
    }
}