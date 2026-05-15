#ifndef BOMBAS_H
#define BOMBAS_H
#include <Arduino.h>

class BombasAPI {
public:
    void iniciar();
    void ligarEncher();
    void ligarEsvaziar();
    void desligar();
};

extern BombasAPI Bombas;
#endif