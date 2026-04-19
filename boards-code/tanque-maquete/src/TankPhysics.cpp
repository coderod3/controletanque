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
    // Lógica: Se 18cm é 0L e 2cm é 2L, a fórmula é:
    float volume = (TANK_HEIGHT_EMPTY - distance) * (2.0 / (TANK_HEIGHT_EMPTY - TANK_HEIGHT_FULL));
    
    // Garante que o volume fique entre 0 e 2 Litros
    return fmax(0.0, fmin(2.0, volume)); 
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