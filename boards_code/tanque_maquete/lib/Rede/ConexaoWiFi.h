#ifndef CONEXAO_WIFI_H
#define CONEXAO_WIFI_H

#include <Arduino.h>

class ConexaoWiFiAPI {
public:
    void iniciar();
    void manter(); // Lógica de reconexão
    bool estaConectado();

private:
    bool _conectado;
    unsigned long _ultimaTentativa;
};

extern ConexaoWiFiAPI ConexaoWiFi;

#endif