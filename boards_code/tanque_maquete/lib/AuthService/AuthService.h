#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <Arduino.h>
#include "Config.h"

class AuthService {
public:
    AuthService();
    
    // Inicializa a lógica de autenticação
    void init();
    
    // Orquestra a leitura da tag e validação na nuvem
    bool update(); 
    
    // Getters de estado
    bool isAuthorized();
    String getActiveUserID();
    String getActiveUserName();

    // Encerra a sessão atual
    void logout();

private:
    bool _authorized;
    String _activeUserID;
    String _activeUserName;
};

// Instância global
extern AuthService auth;

#endif