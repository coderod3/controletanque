#include "AuthService.h"
#include "RFIDReader.h" 
#include "WiFiService.h" 
#include "InputManager.h" // NOVO: Para detectar interações físicas

AuthService::AuthService() : 
    _authorized(false), 
    _isValidating(false), 
    _activeUserID(""), 
    _activeUserName(""),
    _lastActivityTime(0) {}

void AuthService::init() {
    rfidReader.init(); 
    Serial.println("[Auth] Servico de Autenticacao Hibrido Iniciado.");
}

bool AuthService::update() {
    // 1. Monitoramento de Inatividade se já estiver autorizado
    if (_authorized) {
        // Se houver interação em qualquer botão, reseta o temporizador
        if (inputs.isIncClicked() || inputs.isDecClicked() || inputs.isConfClicked() ||
            inputs.isIncPressed() || inputs.isDecPressed() || inputs.isConfPressed()) {
            _lastActivityTime = millis();
        }

        // Se passar de 15 segundos sem atividade, força o Logout automático
        if (millis() - _lastActivityTime > 15000) {
            Serial.println("[Auth] Sessao expirada por inatividade.");
            logout();
            return false;
        }
        return true;
    }

    // 2. Aguardando validação da nuvem
    if (_isValidating) {
        bool isAuth = false;
        String nome = "";
        
        if (connectivity.readAuthResponse(isAuth, nome)) {
            processValidationResponse(isAuth, nome);
        }
        return false; 
    }

    // 3. Varredura física de novas tags
    String uid = rfidReader.readTag();
    if (uid == "") return false;

    // 4. Dispara validação em background
    Serial.println("[Auth] Tag detectada: " + uid + ". Validando na nuvem...");
    _isValidating = true;
    _pendingUID = uid;
    connectivity.queueAuthRequest(uid); 

    return false;
}

void AuthService::processValidationResponse(bool isAuthorized, String userName) {
    _isValidating = false; 

    if (isAuthorized) {
        _authorized = true;
        _activeUserID = _pendingUID;
        _activeUserName = userName;
        _lastActivityTime = millis(); // Inicia contagem de inatividade
        Serial.printf("[Auth] Acesso Liberado. Bem-vindo: %s\n", _activeUserName.c_str());
    } else {
        Serial.println("[Auth] Acesso Negado pela nuvem.");
        _pendingUID = "";
    }
}

void AuthService::logout() {
    _authorized = false;
    _isValidating = false;
    _activeUserID = "";
    _activeUserName = "";
    _lastActivityTime = 0;
    Serial.println("[Auth] Sessao encerrada.");
}

bool AuthService::isAuthorized() { return _authorized; }
bool AuthService::isValidating() { return _isValidating; }
String AuthService::getActiveUserID() { return _activeUserID; }
String AuthService::getActiveUserName() { return _activeUserName; }

AuthService auth;