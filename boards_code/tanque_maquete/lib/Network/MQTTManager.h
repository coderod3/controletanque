#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "Config.h"
#include "NetworkEvents.h"

class MQTTManager {
public:
    // Inicia a configuração do MQTT
    static void init();
    
    // Mantém a conexão viva e processa mensagens (Chamado no loop do Core 0)
    static void handle();
    
    // Verifica se está conectado ao Broker
    static bool isConnected();

    // Funções de publicação direta
    static void publishTelemetria(float nivel, const char* operador);
    static void publishLog(const char* message, bool isError = false);

private:
    static void _mqttCallback(char* topic, byte* payload, unsigned int length);
};

// Precisamos exportar a rxQueue para o MQTTManager poder empurrar os comandos recebidos
extern QueueHandle_t rxQueue;

#endif // MQTT_MANAGER_H