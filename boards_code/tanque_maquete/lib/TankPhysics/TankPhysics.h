#ifndef TANK_PHYSICS_H
#define TANK_PHYSICS_H

#include <Preferences.h> // Necessário para persistência flash
#include "Config.h"      //

class TankPhysics {
public:
    TankPhysics();
    
    // Inicializa sensores e carrega calibração (Prioriza Flash > Config.h)
    void init();
    
    // Realiza uma nova leitura e atualiza a média móvel
    void update();
    
    // Retorna o volume atual em Litros baseado nos parâmetros ativos
    float getVolume();
    
    // Retorna a distância bruta em cm (útil para debug)
    float getRawDistance();

    // Retorna o volume máximo configurado no momento
    float getMaxVolume();

    // Manobra de Sincronização: Grava novos valores na Flash e aplica na RAM
    void syncConfig(float maxVol, float distVazio, float distCheio);

    // Apaga a calibração da Flash e volta a usar os valores do Config.h
    void resetToFactory();

private:
    Preferences _prefs;
    float _currentDistance;
    float _readings[SENSOR_SAMPLES]; //
    int _readIndex;
    float _total;
    float _average;

    // Parâmetros de cálculo ativos (podem ser diferentes dos macros do Config.h)
    float _activeMaxVolume;
    float _activeDistVazio;
    float _activeDistCheio;

    float _measureDistance(); // Lógica interna do sensor
    float _calculateVolume(float distance);
};

// Instância global
extern TankPhysics tank;

#endif