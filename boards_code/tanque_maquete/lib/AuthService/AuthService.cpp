#include "AuthService.h"
#include "RFIDReader.h" 
#include "WiFiService.h" 
#include "InputManager.h" 

AuthService::AuthService() : 
    _authorized(false), 
    _isValidating(false), 
    _activeUserID(""), 
    _activeUserName(""),
    _lastActivityTime(0) {}

void AuthService::init() {
    rfidReader.init(); 
    Serial.println("[Auth] Servico de Autenticacao Edge (Offline) Iniciado.");
}

// FASE 3: Funções de Gestão do Banco Local
void AuthService::syncUser(String uid, String nome, bool ativo) {
    _prefs.begin("users", false); // Abre namespace "users" em modo leitura/escrita
    if (ativo) {
        _prefs.putString(uid.c_str(), nome);
        Serial.println("[Auth] Usuario salvo na Flash: " + uid + " -> " + nome);
    } else {
        _prefs.remove(uid.c_str());
        Serial.println("[Auth] Usuario deletado da Flash: " + uid);
    }
    _prefs.end();
}

void AuthService::clearUsers() {
    _prefs.begin("users", false);
    _prefs.clear();
    _prefs.end();
    Serial.println("[Auth] Todos os usuarios foram apagados da Flash.");
}

bool AuthService::_checkLocalAuth(String uid, String& outName) {
    _prefs.begin("users", true); // Modo somente leitura (muito rápido)
    bool found = _prefs.isKey(uid.c_str());
    if (found) {
        outName = _prefs.getString(uid.c_str(), "Desconhecido");
    }
    _prefs.end();
    return found;
}

bool AuthService::update() {
    // 1. Monitoramento de Inatividade (Logout)
    if (_authorized) {
        if (inputs.isIncClicked() || inputs.isDecClicked() || inputs.isConfClicked() ||
            inputs.isIncPressed() || inputs.isDecPressed() || inputs.isConfPressed()) {
            _lastActivityTime = millis();
        }
        if (millis() - _lastActivityTime > 15000) {
            logout();
            return false;
        }
        return true;
    }

    // 2. Aguardando fallback da nuvem (se aplicável)
    if (_isValidating) {
        bool isAuth = false;
        String nome = "";
        if (connectivity.readAuthResponse(isAuth, nome)) {
            processValidationResponse(isAuth, nome);
        }
        return false; 
    }

    // 3. Varredura física
    String uid = rfidReader.readTag();
    if (uid == "") return false;
    Serial.println("[Auth] Tag detectada: " + uid);

    // --- FASE 3: LÓGICA LOCAL-FIRST (EDGE COMPUTING) ---
    
    // 3.1. Procura na Flash da própria placa (Resolve em < 5ms)
    String nomeLocal;
    if (_checkLocalAuth(uid, nomeLocal)) {
        Serial.println("[Auth] Autorizado localmente (Edge): " + nomeLocal);
        _pendingUID = uid;
        processValidationResponse(true, nomeLocal);
        return true; // Pula a nuvem totalmente!
    }

    // 3.2. Se não está na Flash e não tem rede, nega na hora.
    if (!connectivity.isConnected()) {
        Serial.println("[Auth] Acesso Negado (Offline e nao cadastrado na Flash).");
        _pendingUID = uid;
        processValidationResponse(false, "Nao Cadastrada");
        return false;
    }

    // 3.3. Fallback: Procura na Nuvem
    Serial.println("[Auth] Nao encontrado localmente. Consultando Nuvem...");
    _isValidating = true;
    _pendingUID = uid;
    connectivity.queueAuthRequest(uid); 

    return false;
}

void AuthService::processValidationResponse(bool isAuthorized, String userName) {
    _isValidating = false; 
    _activeUserName = userName;

    if (isAuthorized) {
        _authorized = true;
        _activeUserID = _pendingUID;
        _lastActivityTime = millis();
    } else {
        _activeUserID = _pendingUID; // Guarda para exibir na tela o UID rejeitado
        _pendingUID = "";
    }
}

void AuthService::logout() {
    _authorized = false;
    _isValidating = false;
    _activeUserID = "";
    _activeUserName = "";
    _lastActivityTime = 0;
}

bool AuthService::isAuthorized() { return _authorized; }
bool AuthService::isValidating() { return _isValidating; }
String AuthService::getActiveUserID() { return _activeUserID; }
String AuthService::getActiveUserName() { return _activeUserName; }

AuthService auth;