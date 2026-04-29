#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class CloudSync {
public:
    CloudSync();
    
    // Envia o estado atual para o Gêmeo Digital
    void syncDigitalTwin(String status, float nivel);
    
    // Registra a conclusão de uma tarefa no banco
    void sendAuditLog(String rfid, String acao, float volume, float anterior, float atual);

private:
    const String _baseUrl = "https://controletanque.vercel.app/pages/api"; // Centralize sua URL aqui
};

extern CloudSync cloud; // Instância global para ser usada no main
#endif