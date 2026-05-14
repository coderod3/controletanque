#include "TankPhysics.h"
#include "HardwareMap.h"

TankPhysics::TankPhysics() :
    _currentDistance(18.0f),
    _currentVolume(0.0f),
    _lastReadTime(0),
    _maxVolume(100.0f),
    _distVazio(18.0f),
    _distCheio(2.0f),
    _isFilling(false),
    _isEmptying(false) {}

void TankPhysics::init() {
    pinMode(PIN_TRIGGER, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    digitalWrite(PIN_TRIGGER, LOW);

    prefs.begin("calibra", false);
    _maxVolume = prefs.getFloat("max_v", 100.0f);
    _distVazio = prefs.getFloat("d_vazio", 18.0f);
    _distCheio = prefs.getFloat("d_cheio", 2.0f);
    prefs.end();

    _currentDistance = _getMedianDistance();
    update();

    Serial.println("[Physics] Filtros (Mediana + Slew Rate + Direcional) OK");
}

float TankPhysics::_getMedianDistance() {
    float amostras[SENSOR_SAMPLES];
    
    for (int i = 0; i < SENSOR_SAMPLES; i++) {
        digitalWrite(PIN_TRIGGER, LOW); 
        delayMicroseconds(2);
        digitalWrite(PIN_TRIGGER, HIGH); 
        delayMicroseconds(10);
        digitalWrite(PIN_TRIGGER, LOW);
        
        long dur = pulseIn(PIN_ECHO, HIGH, 35000);
        amostras[i] = (dur == 0) ? _distVazio : (dur * 0.0343f) / 2.0f;
        
        delay(SENSOR_ECHO_DELAY_MS);
    }
    
    // Bubble Sort
    for (int i = 0; i < SENSOR_SAMPLES - 1; i++) {
        for (int j = i + 1; j < SENSOR_SAMPLES; j++) {
            if (amostras[i] > amostras[j]) {
                float temp = amostras[i];
                amostras[i] = amostras[j];
                amostras[j] = temp;
            }
        }
    }
    return amostras[SENSOR_SAMPLES / 2];
}

float TankPhysics::_applyPhysicalLimits(float novaDistancia, float dt_segundos) {
    float mudancaMaxima = SENSOR_MAX_SLEW_RATE_CM_S * dt_segundos;

    if (abs(novaDistancia - _currentDistance) > mudancaMaxima && _currentDistance > 0) {
        return _currentDistance;
    }

    if (_isFilling && novaDistancia > _currentDistance) return _currentDistance;
    if (_isEmptying && novaDistancia < _currentDistance) return _currentDistance;

    return novaDistancia;
}

void TankPhysics::setDirection(bool filling, bool emptying) {
    _isFilling = filling;
    _isEmptying = emptying;
}

void TankPhysics::update() {
    unsigned long agora = millis();
    float dt = (agora - _lastReadTime) / 1000.0f;

    if (dt < (SENSOR_READ_INTERVAL / 1000.0f)) return;   // Use a constante do Config.h

    _lastReadTime = agora;

    float mediana = _getMedianDistance();
    _currentDistance = _applyPhysicalLimits(mediana, dt);

    // Cálculo 0-100%
    if (_currentDistance >= _distVazio) {
        _currentVolume = 0.0f;
    } else if (_currentDistance <= _distCheio) {
        _currentVolume = 100.0f;
    } else {
        _currentVolume = ((_distVazio - _currentDistance) / (_distVazio - _distCheio)) * 100.0f;
    }
}

void TankPhysics::syncConfig(float maxVol, float distVazio, float distCheio) {
    _maxVolume = maxVol;
    _distVazio = distVazio;
    _distCheio = distCheio;

    prefs.begin("calibra", false);
    prefs.putFloat("max_v", _maxVolume);
    prefs.putFloat("d_vazio", _distVazio);
    prefs.putFloat("d_cheio", _distCheio);
    prefs.end();
}

float TankPhysics::getVolume() const { return _currentVolume; }
float TankPhysics::getRawDistance() const { return _currentDistance; }
float TankPhysics::getMaxVolume() const { return _maxVolume; }

TankPhysics tank;