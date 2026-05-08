#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <Arduino.h>

class AuthService {
public:
    AuthService();
    
    void init();
    
    // Chamado no loop principal do Core 1. Retorna true se estiver autorizado.
    bool update();
    
    // Nova função: O Core 1 chamará isso quando a resposta da rede chegar pela fila
    void processValidationResponse(bool isAuthorized, String userName);
    
    void logout();
    
    bool isAuthorized();
    bool isValidating(); // Novo: Para o Display mostrar "Aguarde..."
    String getActiveUserID();
    String getActiveUserName();

private:
    bool _authorized;
    bool _isValidating; // Evita spam de requisições
    String _pendingUID; // Guarda o UID que está sendo validado no momento
    String _activeUserID;
    String _activeUserName;
};

extern AuthService auth;

#endif