#include "GerenciadorMQTT.h"
#include "Rede.h"
#include "Config.h"
#include <WiFiClientSecure.h> // <--- Usando o Cliente com Criptografia SSL
#include <PubSubClient.h>

static WiFiClientSecure espClient; 
static PubSubClient mqttClient(espClient);
GerenciadorMQTTAPI GerenciadorMQTT;

void GerenciadorMQTTAPI::iniciar() {
    // Comando mágico: Aceita a criptografia sem precisar instalar o certificado raiz manualmente
    espClient.setInsecure(); 
    
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(this->_callback);
    
    // Tempo um pouco maior pois o SSL demora mais para responder
    mqttClient.setKeepAlive(15); 

    Serial.println("[MQTT] Servico inicializado (Modo SEGURO SSL - Porta 8883).");
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
    if (millis() - ultimaTentativa < 7000) return; // Dá 7 segundos para o SSL negociar
    ultimaTentativa = millis();

    Serial.print("[MQTT] Conectando ao Broker Seguro... ");
    
    const char* lwtTopic = TOPIC_STATUS;
    const char* lwtMsg = "{\"status\": \"OFFLINE\"}";

    // Agora passamos o Usuário e Senha definidos no Config.h
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, lwtTopic, 0, true, lwtMsg)) {
        Serial.println("OK! Criptografia estabelecida.");
        
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