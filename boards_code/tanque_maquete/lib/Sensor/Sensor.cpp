#include "Sensor.h"
#include "HardwareMap.h"
#include "Config.h" 

SensorAPI Sensor;

void SensorAPI::iniciar() {
    pinMode(PIN_TRIGGER, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    ultimaDistancia = 0.0;
    ultimoTempoLeitura = 0;
    estadoEnchendo = false;
    estadoEsvaziando = false;
}

void SensorAPI::setDirecao(bool enchendo, bool esvaziando) {
    estadoEnchendo = enchendo;
    estadoEsvaziando = esvaziando;
}

float SensorAPI::lerCm() {
    if (millis() - ultimoTempoLeitura < 50) return ultimaDistancia;
    ultimoTempoLeitura = millis();

    digitalWrite(PIN_TRIGGER, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIGGER, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIGGER, LOW);

    long duracao = pulseIn(PIN_ECHO, HIGH, 30000); 
    if (duracao == 0) return ultimaDistancia; 

    float distanciaLida = (duracao * 0.0343f) / 2.0f;

    if (ultimaDistancia > 0.0) {
        // REGRA DIRECIONAL (Efeito Catraca Anti-Ondas)
        // Lembre-se: Encher = Distância diminui. Esvaziar = Distância aumenta.
        if (estadoEnchendo && distanciaLida > ultimaDistancia) {
            distanciaLida = ultimaDistancia; // Ignora o pulo (onda descendo)
        }
        else if (estadoEsvaziando && distanciaLida < ultimaDistancia) {
            distanciaLida = ultimaDistancia; // Ignora o pulo (onda subindo)
        }
        
        // Filtro EMA (Suavização final)
        ultimaDistancia = (0.2f * distanciaLida) + (0.8f * ultimaDistancia);
    } else {
        ultimaDistancia = distanciaLida;
    }

    return ultimaDistancia;
}

float SensorAPI::lerPorcentagem() {
    float cm = lerCm();
    if (cm >= TANK_HEIGHT_EMPTY) return 0.0f;
    if (cm <= TANK_HEIGHT_FULL) return 100.0f;
    return ((TANK_HEIGHT_EMPTY - cm) / (TANK_HEIGHT_EMPTY - TANK_HEIGHT_FULL)) * 100.0f;
}