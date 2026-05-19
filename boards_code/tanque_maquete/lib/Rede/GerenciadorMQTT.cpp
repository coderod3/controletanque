#include "GerenciadorMQTT.h"
#include "Rede.h"
#include "Config.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

static WiFiClientSecure espClient;
static PubSubClient mqttClient(espClient);
GerenciadorMQTTAPI GerenciadorMQTT;
static bool wasConnected = false;

void GerenciadorMQTTAPI::iniciar() {
    espClient.setInsecure(); 
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(this->_callback);
    wasConnected = false;
    Serial.println("[MQTT] Servico inicializado.");
}

bool GerenciadorMQTTAPI::conectado() {
    return mqttClient.connected();
}

void GerenciadorMQTTAPI::processar() {
    if (!mqttClient.connected()) {
        if (wasConnected) {
            // Transitou de conectado para desconectado
            Serial.println("[MQTT] Conexão perdida. Publicando status OFFLINE...");
            wasConnected = false;
        }
        _tentarReconectar();
        return;
    }

    // Se estava desconectado e agora reconectou
    if (!wasConnected) {
        Serial.println("[MQTT] Reconectado! Publicando status ONLINE...");
        mqttClient.publish(TOPIC_TELEMETRIA, "{\"status\": \"ONLINE\", \"timestamp\": \"" + String(millis()) + "\"}");
        wasConnected = true;
    }

    mqttClient.loop();

    MensagemSaida msg;
    if (xQueueReceive(filaTX, &msg, 0) == pdPASS) {
        mqttClient.publish(msg.topico, msg.payload);
        Serial.print("[MQTT] Enviado: ");
        Serial.println(msg.topico);
    }
}

void GerenciadorMQTTAPI::_tentarReconectar() {
    static unsigned long ultimaTentativa = 0;
    if (millis() - ultimaTentativa < 5000) return;
    ultimaTentativa = millis();

    Serial.print("[MQTT] Conectando ao Broker... ");
    
    const char* lwtTopic = TOPIC_TELEMETRIA;
    const char* lwtMsg = "{\"status\": \"OFFLINE\"}";

    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, lwtTopic, 0, true, lwtMsg)) {
        Serial.println("OK!");
        mqttClient.subscribe(TOPIC_COMANDO);
        wasConnected = true;
    } else {
        Serial.print("Falha, rc=");
        Serial.println(mqttClient.state());
        wasConnected = false;
    }
}

void GerenciadorMQTTAPI::_callback(char* topic, byte* payload, unsigned int length) {
    ComandoEntrada cmd;
    memset(cmd.payload, 0, sizeof(cmd.payload));

    unsigned int len = (length < sizeof(cmd.payload) - 1) ? length : sizeof(cmd.payload) - 1;
    memcpy(cmd.payload, payload, len);

    if (filaRX != NULL) {
        xQueueSend(filaRX, &cmd, 0);
    }
}