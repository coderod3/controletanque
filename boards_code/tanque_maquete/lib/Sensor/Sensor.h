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
    void setDirecao(bool enchendo, bool esvaziando); // Avisa o sensor do estado físico
    float lerCm();
    float lerPorcentagem();
};

extern SensorAPI Sensor;
#endif