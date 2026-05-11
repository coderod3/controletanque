#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <Arduino.h>
#include <Preferences.h> // NOVO: Base de dados na Flash

class AuthService {
public:
    AuthService();
    void init();
    
    bool update(); 
    void processValidationResponse(bool isAuthorized, String userName);
    void logout();

    // FASE 3: Sincronização Local (Offline)
    void syncUser(String uid, String nome, bool ativo);
    void clearUsers();

    bool isAuthorized();
    bool isValidating();
    String getActiveUserID();
    String getActiveUserName();

private:
    bool _authorized;
    bool _isValidating;
    String _pendingUID;
    String _activeUserID;
    String _activeUserName;
    unsigned long _lastActivityTime;

    Preferences _prefs; // NOVO: Objeto de armazenamento NVM
    bool _checkLocalAuth(String uid, String& outName); // NOVO: Verificador interno
};

extern AuthService auth;
#endif