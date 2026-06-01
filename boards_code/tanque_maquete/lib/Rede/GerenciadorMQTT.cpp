#include "GerenciadorMQTT.h"
#include "Rede.h"
#include "Config.h"
#include <WiFiClient.h> // <--- Usando Cliente Wi-Fi Normal (Rápido)
#include <PubSubClient.h>

static WiFiClient espClient; // <--- Sem "Secure"
static PubSubClient mqttClient(espClient);
GerenciadorMQTTAPI GerenciadorMQTT;

void GerenciadorMQTTAPI::iniciar() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(this->_callback);
    
    mqttClient.setKeepAlive(10); 

    Serial.println("[MQTT] Servico inicializado (Modo Rapido sem SSL).");
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

    Serial.print("[MQTT] Conectando ao Broker sem criptografia... ");
    
    const char* lwtTopic = TOPIC_STATUS;
    const char* lwtMsg = "{\"status\": \"OFFLINE\"}";

    // Conecta APENAS com o ID e o Testamento (LWT). Sem usuário e senha.
    if (mqttClient.connect(MQTT_CLIENT_ID, lwtTopic, 0, true, lwtMsg)) {
        Serial.println("OK! Instantâneo!");
        
        mqttClient.publish(TOPIC_STATUS, "{\"status\": \"ONLINE\"}", true); 
        mqttClient.subscribe(TOPIC_COMANDO);
        
        ComandoEntrada cmdSync;
        strcpy(cmdSync.payload, "{\"comando\": \"GET_SYNC\"}");
        xQueueSend(filaRX, &cmdSync, 0);
    } else {
        Serial.print("Falha, rc=");
        Serial.println(mqttClient.state());
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