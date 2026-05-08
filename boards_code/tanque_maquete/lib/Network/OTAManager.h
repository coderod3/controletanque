#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <ArduinoOTA.h>
#include <Arduino.h>

class OTAManager {
public:
    // Configura callbacks e inicia o serviço de escuta
    static void init(const char* hostname);
    
    // Deve ser chamado dentro do loop da rede (Core 0)
    static void handle();
};

#endif // OTA_MANAGER_H