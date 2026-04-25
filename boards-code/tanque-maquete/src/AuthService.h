#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <SPI.h>
#include <MFRC522.h>
#include "Config.h" // Corrigido para C maiúsculo

class AuthService {
public:
    AuthService();
    
    void init();
    
    // Agora valida a tag via POST para a Vercel/Neon
    bool update(); 
    
    // Getters para a Máquina de Estados
    bool isAuthorized();
    String getActiveUserID();
    
    // Encerra a sessão
    void logout();

private:
    MFRC522 _mfrc522;
    bool _authorized;
    String _activeUserID;
    // _checkWhitelist removido: a validação agora é externa (API)
};

extern AuthService auth;

#endif