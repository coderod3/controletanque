#include <Arduino.h>
#include <SPI.h> // Importante garantir a inclusão
#include "Config.h"
#include "AuthService.h"

AuthService::AuthService() : _mfrc522(PIN_RFID_SS, PIN_RFID_RST), _authorized(false), _activeUserID("") {}

void AuthService::init() {
    // CORREÇÃO 1: Iniciar o barramento SPI explicitamente com os pinos do Config.h
    SPI.begin(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);
    
    _mfrc522.PCD_Init();
    Serial.println("[Auth] RFID MFRC522 Inicializado.");
}

bool AuthService::update() {
    if (_authorized) return true;

    // Se não houver cartão presente, retorna falso rápido
    if (!_mfrc522.PICC_IsNewCardPresent() || !_mfrc522.PICC_ReadCardSerial()) {
        return false;
    }

    String uid = "";
    for (byte i = 0; i < _mfrc522.uid.size; i++) {
        uid += String(_mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        uid += String(_mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    // Debug para você ver no console exatamente como a string está chegando
    Serial.print("[Auth] Tag detectada: ");
    Serial.println(uid);

    if (_checkWhitelist(uid)) {
        _authorized = true;
        _activeUserID = uid;
        Serial.println("[Auth] ACESSO LIBERADO!");
        return true;
    } else {
        Serial.println("[Auth] ACESSO NEGADO - UID INVALIDO");
        // Dá um pequeno delay para não ficar spamando erro no console
        delay(500); 
    }

    _mfrc522.PICC_HaltA();
    _mfrc522.PCD_StopCrypto1();

    return false;
}

bool AuthService::_checkWhitelist(String uid) {
    // CORREÇÃO 2: Removi os ":" para bater com a string gerada pelo loop
    if (uid == "01020304" || uid == "A1B2C3D4") return true; 
    
    // Tag padrão de muitos kits (também sem os ":")
    if (uid == "83181D1A") return true; 

    return false;
}

void AuthService::logout() {
    _authorized = false;
    _activeUserID = "";
    Serial.println("[Auth] Sessão encerrada.");
}

bool AuthService::isAuthorized() { return _authorized; }
String AuthService::getActiveUserID() { return _activeUserID; }

AuthService auth;