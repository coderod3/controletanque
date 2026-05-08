#ifndef RFID_READER_H
#define RFID_READER_H

#include <Arduino.h>

/**
 * @brief Classe de abstração para leitores RFID.
 * 
 * Esta classe define a interface comum para qualquer hardware RFID (RC522 ou PN532),
 * permitindo que o sistema de autenticação funcione independente do modelo do sensor.
 */
class RFIDReader {
public:
    RFIDReader();
    
    /**
     * @brief Inicializa o barramento SPI e o chip RFID.
     * Utiliza as definições de pinos do HardwareMap.h internamente no .cpp.
     */
    void init();
    
    /**
     * @brief Verifica a presença de uma tag e lê seu UID.
     * 
     * @return String O UID formatado em Hexadecimal (ex: "A1B2C3D4"). 
     * Retorna uma String vazia ("") se nenhuma tag for detectada.
     */
    String readTag();

private:
    // O objeto do driver (MFRC522 ou PN532) será declarado nos arquivos .cpp
    // específicos para manter este cabeçalho limpo.
};

// Instância global para ser usada em todo o projeto
extern RFIDReader rfidReader;

#endif