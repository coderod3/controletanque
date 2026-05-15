#ifndef TELA_H
#define TELA_H
#include <Arduino.h>

class TelaAPI {
private:
    String ultimaLinha1;
    String ultimaLinha2;
    unsigned long ultimaAtualizacao;
public:
    void iniciar();
    void limpar();
    void atualizar(String linha1, String linha2);
};

extern TelaAPI Tela;
#endif