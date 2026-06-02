#include "LeitorRFID.h"
#include "HardwareMap.h"
#include <SPI.h>
#include <Adafruit_PN532.h>

Adafruit_PN532 pn532(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);
LeitorRFIDAPI LeitorRFID;

void LeitorRFIDAPI::iniciar() {
    pn532.begin();
    uint32_t versiondata = pn532.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("ERRO: PN532 nao detectado!");
        return;
    }
    pn532.SAMConfig(); // Prepara para ler tags
}

String LeitorRFIDAPI::lerTag() {
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; 
    uint8_t uidLength; 

    // Timeout de 50ms para não travar o loop
    if (pn532.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50)) {
        String uidStr = "";
        for (uint8_t i = 0; i < uidLength; i++) {
            uidStr += String(uid[i] < 0x10 ? "0" : "");
            uidStr += String(uid[i], HEX);
        }
        uidStr.toUpperCase();
        return uidStr;
    }
    return ""; // Retorna vazio se não houver cartão
}