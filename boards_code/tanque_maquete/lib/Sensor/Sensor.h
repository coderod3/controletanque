#ifndef SENSOR_H
#define SENSOR_H
#include <Arduino.h>

class SensorAPI {
private:
    float ultimaDistancia;
    unsigned long ultimoTempoLeitura;
    bool estadoEnchendo;
    bool estadoEsvaziando;
    
    float amostrasBuffer[10];
    uint8_t indiceAmostra;
    
    float _calcularMediana(float amostras[], uint8_t tamanho);
    float _limitarSlew(float novaDistancia, float ultimaDistancia, float deltaTempoSeg);
public:
    void iniciar();
    void setDirecao(bool enchendo, bool esvaziando);
    float lerCm();
    float lerPorcentagem();
};

extern SensorAPI Sensor;
#endif