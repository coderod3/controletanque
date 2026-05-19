#ifndef CONTROLE_NIVEL_H
#define CONTROLE_NIVEL_H
#include <Arduino.h>

class ControleNivelAPI {
private:
    float alvoCm;
    float alvoVol;
    bool ativo;
    unsigned long ultimaAtualizacao;
public:
    void iniciar();
    void setarAlvo(float centimetros);
    void setarAlvoVolume(float volumeL);
    void parar();
    void atualizar(float distanciaAtual);
    bool estaTrabalhando();
    float obterAlvoVol();
};

extern ControleNivelAPI ControleNivel;
#endif