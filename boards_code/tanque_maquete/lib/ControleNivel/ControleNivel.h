#ifndef CONTROLE_NIVEL_H
#define CONTROLE_NIVEL_H
#include <Arduino.h>

class ControleNivelAPI {
private:
    float alvoLitros;
    bool ativo;
public:
    void iniciar();
    void setarAlvo(float litros);
    float getAlvo(); // NOVO: Permite consultar o alvo atual
    void parar(); 
    void atualizar(float volumeAtualLitros); 
    bool estaTrabalhando();
};

extern ControleNivelAPI ControleNivel;
#endif