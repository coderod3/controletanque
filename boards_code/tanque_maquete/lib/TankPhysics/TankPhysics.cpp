#include <Arduino.h>
//#include "Config.h"
#include "HardwareMap.h" // - Necessário para PIN_TRIGGER, ECHO e SENSOR_SAMPLES
#include "TankPhysics.h"

TankPhysics::TankPhysics() : _readIndex(0), _total(0), _average(0) {
    for (int i = 0; i < SENSOR_SAMPLES; i++) _readings[i] = 18.0; // Inicia como vazio
}

void TankPhysics::init() {
    // 1. Carrega parâmetros da Flash (ou usa fallback do Config.h)
    _prefs.begin("calibra", false);
    _activeMaxVolume = _prefs.getFloat("max_v", TANK_MAX_VOLUME);
    _activeDistVazio = _prefs.getFloat("d_vazio", TANK_HEIGHT_EMPTY);
    _activeDistCheio = _prefs.getFloat("d_cheio", TANK_HEIGHT_FULL);
    _prefs.end();

    // 2. Inicialização do Hardware
    pinMode(PIN_TRIGGER, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    digitalWrite(PIN_TRIGGER, LOW);
    
    // 3. Warm-up: Preenche o array com a leitura real inicial
    float initialDist = _measureDistance();
    for(int i = 0; i < SENSOR_SAMPLES; i++) {
        _readings[i] = initialDist;
    }
    _total = initialDist * SENSOR_SAMPLES;
    _average = initialDist;

    Serial.println("[Physics] Sistema de calibração híbrida ativo.");
}

float TankPhysics::_calculateVolume(float distance) {
    // Se a distância for maior que o limite de vazio, volume é 0
    if (distance >= _activeDistVazio) return 0.0;
    
    // Se a distância for menor que o limite de cheio, volume é o máximo
    if (distance <= _activeDistCheio) return _activeMaxVolume;

    // Cálculo linear baseado nos parâmetros ativos
    float range = _activeDistVazio - _activeDistCheio;
    float filled = _activeDistVazio - distance;
    float volume = (filled / range) * _activeMaxVolume;
    
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

void TankPhysics::syncConfig(float maxVol, float distVazio, float distCheio) {
    // 1. Persiste na Flash para o próximo reboot (Independência Offline)
    _prefs.begin("calibra", false);
    _prefs.putFloat("max_v", maxVol);
    _prefs.putFloat("d_vazio", distVazio);
    _prefs.putFloat("d_cheio", distCheio);
    _prefs.end();

    // 2. Atualiza imediatamente em tempo de execução
    _activeMaxVolume = maxVol;
    _activeDistVazio = distVazio;
    _activeDistCheio = distCheio;
    
    Serial.println("[Physics] Calibração sincronizada com sucesso.");
}

float TankPhysics::getVolume() { return _calculateVolume(_average); }
float TankPhysics::getRawDistance() { return _average; }
float TankPhysics::getMaxVolume() { return _activeMaxVolume; }

TankPhysics tank;