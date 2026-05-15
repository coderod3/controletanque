#ifndef LEITOR_RFID_H
#define LEITOR_RFID_H
#include <Arduino.h>

class LeitorRFIDAPI {
public:
    void iniciar();
    String lerTag();
};

extern LeitorRFIDAPI LeitorRFID;
#endif