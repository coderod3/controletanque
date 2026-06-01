#include "GerenciadorMQTT.h"
#include "Rede.h"
#include "Config.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

static WiFiClientSecure espClient;
static PubSubClient mqttClient(espClient);
GerenciadorMQTTAPI GerenciadorMQTT;

void GerenciadorMQTTAPI::iniciar() {
    espClient.setInsecure(); 
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(this->_callback);
    
    // NOVO: Força o HiveMQ a ser impaciente. 
    // Se a placa sumir por 5 segundos, ele dispara o LWT.
    mqttClient.setKeepAlive(10); 

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
    
    // CORREÇÃO: Tópico de Status exclusivo para o Testamento (LWT)
    const char* lwtTopic = "tanque/status";
    const char* lwtMsg = "{\"status\": \"OFFLINE\"}";

    // Conecta pedindo ao broker para reter a mensagem de morte
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, lwtTopic, 0, true, lwtMsg)) {
        Serial.println("OK!");
        
        // Assim que conecta, subscreve a mensagem retida mandando um ONLINE
        mqttClient.publish("tanque/status", "{\"status\": \"ONLINE\"}", true); 
        
        mqttClient.subscribe(TOPIC_COMANDO);
        // Força a placa a injetar um JSON virtual na fila dela pedindo o GET_SYNC para se auto-publicar
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