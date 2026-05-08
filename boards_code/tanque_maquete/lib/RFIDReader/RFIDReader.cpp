#include <Arduino.h>
#include <Adafruit_PN532.h>
#include "RFIDReader.h"
#include "HardwareMap.h"

// Driver para o hardware REAL PN532 via SPI
Adafruit_PN532 _pn532(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);

RFIDReader::RFIDReader() {}

void RFIDReader::init() {
    _pn532.begin();

    uint32_t versiondata = _pn532.getFirmwareVersion();
    if (!versiondata) {
        // Se aparecer isso no monitor serial, os fios estão errados ou sem energia
        Serial.println("[RFID] ERRO: PN532 nao encontrado! Revise a fiacao.");
        return;
    }

    // Configura o módulo para ler tags
    _pn532.SAMConfig();
    Serial.println("[RFID] Driver PN532 (REAL) pronto.");
}

String RFIDReader::readTag() {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; 
    uint8_t uidLength; 

    // Tenta ler uma tag com timeout de 50ms
    success = _pn532.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50);

    if (success) {
        String uidStr = "";
        for (uint8_t i = 0; i < uidLength; i++) {
            uidStr += String(uid[i] < 0x10 ? "0" : "");
            uidStr += String(uid[i], HEX);
        }
        uidStr.toUpperCase();
        return uidStr;
    }

    return "";
}

RFIDReader rfidReader;