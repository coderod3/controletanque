#include "ControleNivel.h"
#include "Bombas.h"

ControleNivelAPI ControleNivel;

void ControleNivelAPI::iniciar() {
    alvoCm = 0.0f;
    ativo = false;
}

void ControleNivelAPI::setarAlvo(float centimetros) {
    alvoCm = centimetros;
    ativo = true;
}

void ControleNivelAPI::parar() {
    ativo = false;
    Bombas.desligar();
}

bool ControleNivelAPI::estaTrabalhando() {
    return ativo;
}

void ControleNivelAPI::atualizar(float nivelAtual) {
    if (!ativo) return;

    float margem = 0.5f; // 1% de margem de erro

    if (nivelAtual < alvoCm - margem) {
        Bombas.ligarEncher();   // Tem menos água que o alvo -> ENCHE
    } 
    else if (nivelAtual > alvoCm + margem) {
        Bombas.ligarEsvaziar(); // Tem mais água que o alvo -> ESVAZIA
    } 
    else {
        Bombas.desligar();      // Chegou!
        ativo = false; 
    }
}