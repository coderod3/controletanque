#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "Config.h"

class WiFiService {
public:
    WiFiService();
    
    // Inicia a conexão e dispara a tarefa no Core 0
    void init();
    
    // Status para o Display e Máquina de Estados (Core 1)
    bool isConnected();
    
    // Envia o nível do tanque (Telemetria)
    void publishTelemetria(float nivel, String operador);

    // Mantido para compatibilidade com o resto do sistema
    // Agora envia via MQTT no tópico de telemetria/eventos
    bool queueLog(String message, bool isError = false);
    
    // Retorna o comando vindo do Dashboard (ex: "LIGAR", "PARAR")
    // O Core 1 chama isso para saber o que o site mandou
    String getPendingCommand();
    
    bool queueAuthRequest(String rfid_uid);
    bool readAuthResponse(bool& isAuthorized, String& userName);

private:
    // O loop de rede que rodará no Core 0
    static void _networkTask(void* pvParameters);
    
    // Função obrigatória para o MQTT ouvir o Broker
    static void _mqttCallback(char* topic, byte* payload, unsigned int length);
    
    bool _connected;
    
    // Variáveis estáticas para comunicação segura entre núcleos
    static String _lastCommand;
    static bool _hasNewCommand;

    // Buffer de logs (Mantido caso você queira persistência offline)
    static const int LOG_BUFFER_SIZE = 20;
    String _logBuffer[LOG_BUFFER_SIZE];
    int _head = 0;
    int _tail = 0;
};

// Instância global para ser usada em todo o projeto
extern WiFiService connectivity;
extern QueueHandle_t authRxQueue; // Nova fila para respostas do RFID
#endif