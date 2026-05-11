#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h> // <-- AQUI ESTÁ A CORREÇÃO: Necessário para a NVS

class CloudSync {
public:
    CloudSync();
    
    // Envia o estado atual para o Gêmeo Digital
    void syncDigitalTwin(String status, float nivel);
    
    // Registra a conclusão de uma tarefa no banco
    void sendAuditLog(String rfid, String acao, float volume, float anterior, float atual);
    
    // Nova função para bater na API da Vercel
    bool authenticateTag(String rfid_uid, String& outName);
    
    // FASE 3: Edge Computing (Retenção de Logs)
    bool flushNextOfflineLog();
    
private:
    const String _baseUrl = "https://controletanque.vercel.app/pages/api"; // Centralize sua URL aqui

    Preferences _prefs; // Objeto de acesso à NVS
    
    void saveLogOffline(String jsonPayload);

};

extern CloudSync cloud; // Instância global para ser usada no main
#endif