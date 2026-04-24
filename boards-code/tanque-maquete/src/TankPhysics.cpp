#include <Arduino.h>
#include "Config.h"
#include "TankPhysics.h"

TankPhysics::TankPhysics() : _readIndex(0), _total(0), _average(0) {
    for (int i = 0; i < SENSOR_SAMPLES; i++) _readings[i] = 18.0; // Inicia como vazio
}

void TankPhysics::init() {
    pinMode(PIN_TRIGGER, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    digitalWrite(PIN_TRIGGER, LOW);
    
    // Warm-up: Preenche o array com leituras reais imediatas
    // para não começar com média zero (que indicaria tanque cheio)
    float initialDist = _measureDistance();
    for(int i = 0; i < SENSOR_SAMPLES; i++) {
        _readings[i] = initialDist;
    }
    _total = initialDist * SENSOR_SAMPLES;
    _average = initialDist;
}

float TankPhysics::_calculateVolume(float distance) {
    // Se a distância for maior que o vazio, volume é 0
    if (distance >= TANK_HEIGHT_EMPTY) return 0.0;
    
    // Se a distância for menor que o cheio (topo), volume é o máximo
    if (distance <= TANK_HEIGHT_FULL) return TANK_MAX_VOLUME;

    // Cálculo: (100 - Distância Atual) * 1.0
    // Exemplo: Distância 30cm -> (100 - 30) * 1 = 70 Litros
    float volume = (TANK_HEIGHT_EMPTY - distance) * TANK_BASE_AREA;
    
    return volume;
}

void TankPhysics::update() {
    _total = _total - _readings[_readIndex];
    _readings[_readIndex] = _measureDistance();
    _total = _total + _readings[_readIndex];
    _readIndex = (_readIndex + 1) % SENSOR_SAMPLES;
    _average = _total / SENSOR_SAMPLES;
}

float TankPhysics::_measureDistance() {
    digitalWrite(PIN_TRIGGER, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIGGER, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIGGER, LOW);

    long duration = pulseIn(PIN_ECHO, HIGH, 30000);
    float distance = (duration * 0.0343) / 2;
    
    if (distance <= 0) return 18.0; 
    return distance;
}

float TankPhysics::getVolume() { return _calculateVolume(_average); }
float TankPhysics::getRawDistance() { return _average; }

TankPhysics tank;