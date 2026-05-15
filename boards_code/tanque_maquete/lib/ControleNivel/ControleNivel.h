#ifndef CONTROLE_NIVEL_H
#define CONTROLE_NIVEL_H
#include <Arduino.h>

class ControleNivelAPI {
private:
    float alvoCm;
    float alvoVol;
    bool ativo;
public:
    void iniciar();
    void setarAlvo(float centimetros);
    void parar(); // Corte de emergência
    void atualizar(float distanciaAtual); // Chamado a cada ciclo pelo main
    bool estaTrabalhando();
};

extern ControleNivelAPI ControleNivel;
#endif