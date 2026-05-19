#include "ControleNivel.h"
#include "Bombas.h"

ControleNivelAPI ControleNivel;

void ControleNivelAPI::iniciar() {
    alvoCm = 0.0f;
    alvoVol = 0.0f;
    ativo = false;
    ultimaAtualizacao = 0;
}

void ControleNivelAPI::setarAlvo(float centimetros) {
    alvoCm = centimetros;
    ativo = true;
    ultimaAtualizacao = millis();
}

void ControleNivelAPI::setarAlvoVolume(float volumeL) {
    alvoVol = volumeL;
}

void ControleNivelAPI::parar() {
    ativo = false;
    Bombas.desligar();
    ultimaAtualizacao = 0;
}

bool ControleNivelAPI::estaTrabalhando() {
    return ativo;
}

float ControleNivelAPI::obterAlvoVol() {
    return alvoVol;
}

void ControleNivelAPI::atualizar(float nivelAtual) {
    if (!ativo) return;
    
    unsigned long agora = millis();
    if (agora - ultimaAtualizacao < 100) return;
    ultimaAtualizacao = agora;

    float margem = 0.5f;

    if (nivelAtual < alvoCm - margem) {
        Bombas.ligarEncher();
    } 
    else if (nivelAtual > alvoCm + margem) {
        Bombas.ligarEsvaziar();
    } 
    else {
        Bombas.desligar();
        ativo = false;
    }
}