#ifndef TANK_PHYSICS_H
#define TANK_PHYSICS_H

#include <Preferences.h>
#include "Config.h"
#include "HardwareMap.h"

class TankPhysics {
public:
    TankPhysics();
    void init();
    
    void update();
    void setDirection(bool filling, bool emptying);
    
    float getVolume() const;       // 0 a 100%
    float getRawDistance() const;
    float getMaxVolume() const;
    
    void syncConfig(float maxVol, float distVazio, float distCheio);

private:
    float _currentDistance;
    float _currentVolume;
    unsigned long _lastReadTime;
    
    float _maxVolume;
    float _distVazio;
    float _distCheio;

    bool _isFilling;
    bool _isEmptying;

    float _getMedianDistance();
    float _applyPhysicalLimits(float novaDistancia, float dt_segundos);

    Preferences prefs;   // Movido para dentro da classe
};

extern TankPhysics tank;
#endif