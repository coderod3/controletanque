#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <SPI.h>
#include <MFRC522.h>
#include "config.h"

class AuthService {
public:
    AuthService();
    
    void init();
    
    // Verifica se há uma tag presente e valida contra a "lista branca"
    bool update(); 
    
    // Getters para a Máquina de Estados
    bool isAuthorized();
    String getActiveUserID();
    
    // Encerra a sessão (após a operação ou por timeout)
    void logout();

private:
    MFRC522 _mfrc522;
    bool _authorized;
    String _activeUserID;
    
    // Simulação de banco de dados local (pode ser expandido para consulta via WiFiService)
    bool _checkWhitelist(String uid);
};

extern AuthService auth;

#endif