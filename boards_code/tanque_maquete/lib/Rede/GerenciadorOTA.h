#ifndef GERENCIADOR_OTA_H
#define GERENCIADOR_OTA_H

#include <Arduino.h>

class GerenciadorOTAAPI {
public:
    void iniciar();
    void lidar(); // Processa a escuta de novos firmwares
};

extern GerenciadorOTAAPI GerenciadorOTA;

#endif