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
    indiceAmostra = 0;
    for (uint8_t i = 0; i < 10; i++) {
        amostrasBuffer[i] = 0.0;
    }
}

void SensorAPI::setDirecao(bool enchendo, bool esvaziando) {
    estadoEnchendo = enchendo;
    estadoEsvaziando = esvaziando;
}

float SensorAPI::_calcularMediana(float amostras[], uint8_t tamanho) {
    for (uint8_t i = 0; i < tamanho - 1; i++) {
        for (uint8_t j = 0; j < tamanho - i - 1; j++) {
            if (amostras[j] > amostras[j + 1]) {
                float temp = amostras[j];
                amostras[j] = amostras[j + 1];
                amostras[j + 1] = temp;
            }
        }
    }
    return amostras[tamanho / 2];
}

float SensorAPI::_limitarSlew(float novaDistancia, float ultimaDistancia, float deltaTempoSeg) {
    if (deltaTempoSeg <= 0.001f) return novaDistancia;
    
    float deltaMax = SENSOR_MAX_SLEW_RATE_CM_S * deltaTempoSeg;
    float delta = novaDistancia - ultimaDistancia;
    
    if (delta > deltaMax) {
        return ultimaDistancia + deltaMax;
    } else if (delta < -deltaMax) {
        return ultimaDistancia - deltaMax;
    }
    return novaDistancia;
}

float SensorAPI::lerCm() {
    if (millis() - ultimoTempoLeitura < SENSOR_READ_INTERVAL) {
        return ultimaDistancia;
    }
    
    unsigned long tempoAtual = millis();
    ultimoTempoLeitura = tempoAtual;

    digitalWrite(PIN_TRIGGER, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIGGER, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIGGER, LOW);

    long duracao = pulseIn(PIN_ECHO, HIGH, 30000); 
    if (duracao == 0) return ultimaDistancia;
    
    delay(SENSOR_ECHO_DELAY_MS / 2);

    float distanciaLida = (duracao * 0.0343f) / 2.0f;

    if (ultimaDistancia > 0.0) {
        // Regra direcional: ignora ruído baseado na direção do fluxo
        if (estadoEnchendo && distanciaLida > ultimaDistancia) {
            distanciaLida = ultimaDistancia;
        }
        else if (estadoEsvaziando && distanciaLida < ultimaDistancia) {
            distanciaLida = ultimaDistancia;
        }
        
        // Aplicar filtro de slew rate
        float deltaTempoSeg = (millis() - tempoAtual) / 1000.0f;
        distanciaLida = _limitarSlew(distanciaLida, ultimaDistancia, deltaTempoSeg);
    }

    amostrasBuffer[indiceAmostra] = distanciaLida;
    indiceAmostra = (indiceAmostra + 1) % SENSOR_SAMPLES;

    float mediana = _calcularMediana(amostrasBuffer, SENSOR_SAMPLES);
    
    // EMA final com alpha reduzido para estabilização
    ultimaDistancia = (0.15f * mediana) + (0.85f * ultimaDistancia);

    return ultimaDistancia;
}

float SensorAPI::lerPorcentagem() {
    float cm = lerCm();
    if (cm >= TANK_HEIGHT_EMPTY) return 0.0f;
    if (cm <= TANK_HEIGHT_FULL) return 100.0f;
    return ((TANK_HEIGHT_EMPTY - cm) / (TANK_HEIGHT_EMPTY - TANK_HEIGHT_FULL)) * 100.0f;
}