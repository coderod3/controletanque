#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <WiFi.h>
#include "config.h"

class WiFiService {
public:
    WiFiService();
    
    // Inicia a tarefa no Core 0
    void init();
    
    // Status para a Máquina de Estados
    bool isConnected();
    
    // Adiciona log à fila de envio (Thread-Safe)
    void queueLog(String message);

private:
    static void _networkTask(void* pvParameters);
    bool _connected;
    
    // Fila interna para logs offline (Buffer Circular Simples)
    static const int LOG_BUFFER_SIZE = 20;
    String _logBuffer[LOG_BUFFER_SIZE];
    int _head = 0;
    int _tail = 0;
};

extern WiFiService connectivity;

#endif