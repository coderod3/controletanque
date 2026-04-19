#ifndef TANK_PHYSICS_H
#define TANK_PHYSICS_H

#include "config.h"

class TankPhysics {
public:
    TankPhysics();
    
    // Inicializa os pinos e sensores configurados
    void init();
    
    // Realiza uma nova leitura e atualiza a média móvel
    void update();
    
    // Retorna o volume atual em Litros
    float getVolume();
    
    // Retorna a distância bruta em cm (útil para debug)
    float getRawDistance();

private:
    float _currentDistance;
    float _readings[SENSOR_SAMPLES]; // Array para média móvel
    int _readIndex;
    float _total;
    float _average;

    float _measureDistance(); // Lógica interna do sensor
    float _calculateVolume(float distance);
};

// Instância global para ser usada em outros módulos
extern TankPhysics tank;

#endif