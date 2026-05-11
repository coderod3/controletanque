#ifndef TANK_PHYSICS_H
#define TANK_PHYSICS_H

#include <Preferences.h> // Necessário para persistência flash
#include "Config.h"      //
#include "HardwareMap.h" // - Necessário para SENSOR_SAMPLES

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
    // Variáveis de estado
    float _currentDistance;
    float _currentVolume;
    unsigned long _lastReadTime;
    
    // Parâmetros de Calibração (Memória Flash)
    float _maxVolume;
    float _distVazio;
    float _distCheio;

    // Métodos internos dos filtros
    float _getMedianDistance(); 
    float _applyPhysicalLimits(float novaDistancia, float dt_segundos);
};

// Instância global
extern TankPhysics tank;

#endif