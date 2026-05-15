#ifndef BOTOES_H
#define BOTOES_H
#include <Arduino.h>

enum ComandoBotao { NENHUM, MAIS, MENOS, CONFIRMA }; // <-- Mudei OK para CONFIRMA

class BotoesAPI {
private:
    unsigned long ultimoDebounce;
public:
    void iniciar();
    ComandoBotao ler();
};

extern BotoesAPI Botoes;
#endif