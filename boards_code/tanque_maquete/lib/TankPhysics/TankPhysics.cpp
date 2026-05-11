#include "TankPhysics.h"
#include "HardwareMap.h"
#include <Preferences.h> // Biblioteca nativa do ESP32 para salvar dados

Preferences prefs; // Objeto global para acessar a memória Flash

TankPhysics::TankPhysics() : 
    _currentDistance(18.0), 
    _currentVolume(0.0), 
    _lastReadTime(0),
    _maxVolume(100.0), 
    _distVazio(18.0), 
    _distCheio(2.0) {}

void TankPhysics::init() {
    // 1. Inicialização do Hardware
    pinMode(PIN_TRIGGER, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    digitalWrite(PIN_TRIGGER, LOW);

    // 2. Carrega parâmetros da Flash (Se não existirem, usa os padrões)
    prefs.begin("calibra", false);
    _maxVolume = prefs.getFloat("max_v", 100.0);
    _distVazio = prefs.getFloat("d_vazio", 18.0);
    _distCheio = prefs.getFloat("d_cheio", 2.0);
    prefs.end();

    // 3. Primeira leitura forçada para inicializar a máquina
    _currentDistance = _getMedianDistance();
    update();

    Serial.println("[Physics] Sistema de Filtro de Mediana e Calibracao Iniciados.");
}

// --- FILTROS INDUSTRIAIS ---

float TankPhysics::_getMedianDistance() {
    float amostras[5];
    for (int i = 0; i < 5; i++) {
        // Lê 5 vezes (ignora bolhas e insetos passageiros)
        digitalWrite(PIN_TRIGGER, LOW); delayMicroseconds(2);
        digitalWrite(PIN_TRIGGER, HIGH); delayMicroseconds(10);
        digitalWrite(PIN_TRIGGER, LOW);
        
        long dur = pulseIn(PIN_ECHO, HIGH, 30000);
        amostras[i] = (dur == 0) ? _distVazio : (dur * 0.0343) / 2.0;
        delay(5);
    }
    
    // Organiza as amostras do menor para o maior (Bubble Sort)
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 5; j++) {
            if (amostras[i] > amostras[j]) {
                float temp = amostras[i]; 
                amostras[i] = amostras[j]; 
                amostras[j] = temp;
            }
        }
    }
    // Retorna exatamente a leitura do meio, eliminando os extremos anómalos
    return amostras[2]; 
}

float TankPhysics::_applyPhysicalLimits(float novaDistancia, float dt_segundos) {
    float taxaMaximaCmPorSeg = 15.0; // Ajuste se a água subir muito rápido fisicamente
    float mudancaMaxima = taxaMaximaCmPorSeg * dt_segundos;
    
    // Se o salto de leitura for fisicamente impossível, mantém a leitura anterior
    if (abs(novaDistancia - _currentDistance) > mudancaMaxima && _currentDistance > 0) {
        return _currentDistance; 
    }
    return novaDistancia;
}

// --- ATUALIZAÇÃO E CÁLCULO ---

void TankPhysics::update() {
    unsigned long agora = millis();
    float dt = (agora - _lastReadTime) / 1000.0;
    
    // Limita o cálculo a 10 vezes por segundo para não sobrecarregar a CPU
    if (dt < 0.1) return; 
    _lastReadTime = agora;

    // Aplica os dois filtros
    float mediana = _getMedianDistance();
    _currentDistance = _applyPhysicalLimits(mediana, dt);

    // Converte a distância filtrada em Volume
    if (_currentDistance >= _distVazio) {
        _currentVolume = 0.0;
    } else if (_currentDistance <= _distCheio) {
        _currentVolume = _maxVolume;
    } else {
        _currentVolume = ((_distVazio - _currentDistance) / (_distVazio - _distCheio)) * _maxVolume;
    }
}

void TankPhysics::syncConfig(float maxVol, float distVazio, float distCheio) {
    // Atualiza as variáveis em memória
    _maxVolume = maxVol;
    _distVazio = distVazio;
    _distCheio = distCheio;

    // Guarda na Flash do ESP32 para sobreviver à queda de energia
    prefs.begin("calibra", false);
    prefs.putFloat("max_v", _maxVolume);
    prefs.putFloat("d_vazio", _distVazio);
    prefs.putFloat("d_cheio", _distCheio);
    prefs.end();

    Serial.println("[Physics] Calibracao atualizada e salva na memoria Flash.");
}

// --- MÉTODOS PÚBLICOS ---

float TankPhysics::getVolume() { return _currentVolume; }
float TankPhysics::getRawDistance() { return _currentDistance; }
float TankPhysics::getMaxVolume() { return _maxVolume; }

TankPhysics tank;