#ifndef GERENCIADOR_MQTT_H
#define GERENCIADOR_MQTT_H

#include <Arduino.h>

class GerenciadorMQTTAPI {
public:
    void iniciar();
    void processar(); // Gerencia conexão e filas (TX/RX)
    bool conectado();

private:
    void _tentarReconectar();
    static void _callback(char* topic, byte* payload, unsigned int length);
};

extern GerenciadorMQTTAPI GerenciadorMQTT;

#endif