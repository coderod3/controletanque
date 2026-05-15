#include "Bombas.h"
#include "HardwareMap.h" 

BombasAPI Bombas;

void BombasAPI::iniciar() {
    pinMode(PIN_BOMBA_ENCHER, OUTPUT);
    pinMode(PIN_BOMBA_ESVAZ, OUTPUT);
    desligar();
}

void BombasAPI::ligarEncher() {
    digitalWrite(PIN_BOMBA_ENCHER, HIGH);
    digitalWrite(PIN_BOMBA_ESVAZ, LOW);
}

void BombasAPI::ligarEsvaziar() {
    digitalWrite(PIN_BOMBA_ENCHER, LOW);
    digitalWrite(PIN_BOMBA_ESVAZ, HIGH);
}

void BombasAPI::desligar() {
    digitalWrite(PIN_BOMBA_ENCHER, LOW);
    digitalWrite(PIN_BOMBA_ESVAZ, LOW);
}