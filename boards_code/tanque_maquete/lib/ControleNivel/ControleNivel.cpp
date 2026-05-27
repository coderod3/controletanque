#include "ControleNivel.h"
#include "Bombas.h"

ControleNivelAPI ControleNivel;

void ControleNivelAPI::iniciar() {
    alvoLitros = 0.0f;
    ativo = false;
}

void ControleNivelAPI::setarAlvo(float litros) {
    alvoLitros = litros;
    ativo = true;
}

// NOVO: Retorna o alvo exato para o display LCD
float ControleNivelAPI::getAlvo() {
    return alvoLitros;
}

void ControleNivelAPI::parar() {
    ativo = false;
    Bombas.desligar();
}

bool ControleNivelAPI::estaTrabalhando() {
    return ativo;
}

void ControleNivelAPI::atualizar(float volumeAtualLitros) {
    if (!ativo) return;

    float margem = 0.5f; // Margem de erro de meio litro (0.5L) para evitar repique nas válvulas

    if (volumeAtualLitros < (alvoLitros - margem)) {
        Bombas.ligarEncher();   // Falta água para chegar no alvo -> ENCHE
    } 
    else if (volumeAtualLitros > (alvoLitros + margem)) {
        Bombas.ligarEsvaziar(); // Tem mais água que o alvo -> ESVAZIA
    } 
    else {
        Bombas.desligar();      // Chegou na meta!
        ativo = false; 
    }
}