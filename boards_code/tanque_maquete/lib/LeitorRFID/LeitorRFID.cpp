#include "LeitorRFID.h"
#include "HardwareMap.h"
#include <SPI.h>
#include <MFRC522.h>

// O RC522 exige um pino de Reset (RST). 
// Se não existir no seu HardwareMap.h, ele usará o pino 22 por padrão.
#ifndef PIN_RFID_RST
#define PIN_RFID_RST 22 
#endif

// O construtor do MFRC522 no ESP32 assume os pinos SPI padrão do Hardware (MISO=19, MOSI=23, SCK=18)
MFRC522 mfrc522(PIN_RFID_SS, PIN_RFID_RST);

LeitorRFIDAPI LeitorRFID;

void LeitorRFIDAPI::iniciar() {
    SPI.begin(); // Inicia o barramento SPI
    mfrc522.PCD_Init(); // Inicializa o MFRC522
    
    Serial.println("[RFID] Emulador RC522 inicializado.");
}

String LeitorRFIDAPI::lerTag() {
    // 1. Verifica se há um novo cartão próximo ao leitor (Não trava o loop)
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return "";
    }
    
    // 2. Tenta realizar a leitura do número de série do cartão
    if (!mfrc522.PICC_ReadCardSerial()) {
        return "";
    }

    // 3. Monta a String em Hexadecimal
    String uidStr = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        uidStr += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        uidStr += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidStr.toUpperCase();

    // 4. Libera o cartão para evitar múltiplas leituras simultâneas seguidas
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return uidStr;
}