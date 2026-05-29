#ifndef COMANDOS_H
#define COMANDOS_H

#include <Arduino.h>
#include "Rede.h" // Para acessar a estrutura ComandoEntrada

class ComandosAPI {
public:
    void iniciar();
    void monitorar(); // Verifica se há novos comandos na fila de rede
    bool pendenteSync = false; // <--- ADICIONE ESTA FLAG
private:
    void _processar(String json);
};

extern ComandosAPI Comandos;

#endif