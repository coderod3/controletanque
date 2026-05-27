#ifndef SENSOR_H
#define SENSOR_H
#include <Arduino.h>

class SensorAPI {
private:
    float ultimaDistancia;
    unsigned long ultimoTempoLeitura;
    bool estadoEnchendo;
    bool estadoEsvaziando;
public:
    void iniciar();
    void setDirecao(bool enchendo, bool esvaziando);
    float lerCm();
    float lerPorcentagem();
    float lerLitros(); // NOVO: Retorna o volume exato no tanque
};

extern SensorAPI Sensor;
#endif