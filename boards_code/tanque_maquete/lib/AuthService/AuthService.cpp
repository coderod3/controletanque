#include "AuthService.h"
#include "RFIDReader.h" // Depende apenas da abstração do hardware local
#include "WiFiService.h" // Para usar as filas de rede

AuthService::AuthService() : _authorized(false), _isValidating(false), _activeUserID(""), _activeUserName("") {}

void AuthService::init() {
    rfidReader.init(); 
    Serial.println("[Auth] Serviço de Autenticação Híbrido Iniciado.");
}

bool AuthService::update() {
    // 1. Já está autorizado? Segue a vida.
    if (_authorized) return true;

    // 2. Está esperando a Vercel responder? Checa a fila assíncrona.
    if (_isValidating) {
        bool isAuth = false;
        String nome = "";
        
        // Pergunta ao WiFiService se a resposta já chegou (não trava o loop)
        if (connectivity.readAuthResponse(isAuth, nome)) {
            processValidationResponse(isAuth, nome);
        }
        return false; // Continua false até a resposta chegar e ser positiva
    }

    // 3. Lê o Hardware Físico
    String uid = rfidReader.readTag();

    // Se não passou cartão nenhum, sai instantaneamente.
    if (uid == "") return false;

    // 4. Cartão passado! Joga na fila de rede e entra em estado de espera
    Serial.println("[Auth] Tag detectada: " + uid + ". Validando na nuvem em background...");
    
    _isValidating = true;
    _pendingUID = uid;
    
    connectivity.queueAuthRequest(uid); 

    return false;
}

void AuthService::processValidationResponse(bool isAuthorized, String userName) {
    _isValidating = false; // Libera o leitor para novas tentativas

    if (isAuthorized) {
        _authorized = true;
        _activeUserID = _pendingUID;
        _activeUserName = userName;
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
    Serial.println("[Auth] Sessão encerrada.");
}

bool AuthService::isAuthorized() { return _authorized; }
bool AuthService::isValidating() { return _isValidating; }
String AuthService::getActiveUserID() { return _activeUserID; }
String AuthService::getActiveUserName() { return _activeUserName; }

AuthService auth;