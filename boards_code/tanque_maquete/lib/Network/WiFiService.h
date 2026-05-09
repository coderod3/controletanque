#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <WiFi.h>
#include "Config.h"
#include "NetworkEvents.h" // Necessário para struct IncomingCommand e NetworkEvent

class WiFiService {
public:
    WiFiService();
    
    // Inicia a conexão e dispara a tarefa no Core 0
    void init();
    
    // Status para o Display e Máquina de Estados (Core 1)
    bool isConnected();
    
    // Funções de envio para a Fila (Core 1 -> Core 0)
    bool queueTelemetria(float nivel, String operador);
    bool queueDigitalTwin(String status, float nivel);
    bool queueAuditLog(String rfid, String acao, float volume, float anterior, float atual);
    bool queueLog(String message, bool isError = false);
    bool queueAuthRequest(String rfid_uid);
    
    // Funções de leitura de Fila (Core 0 -> Core 1)
    bool readPendingCommand(IncomingCommand& outCommand);
    bool readAuthResponse(bool& isAuthorized, String& userName);

private:
    // O loop de rede que rodará no Core 0
    static void _networkTask(void* pvParameters);
    
    bool _connected;
};

// Instâncias globais para comunicação entre os núcleos
extern WiFiService connectivity;
extern QueueHandle_t authRxQueue; 
#endif