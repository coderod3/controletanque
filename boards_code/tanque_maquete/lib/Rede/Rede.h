#ifndef REDE_H
#define REDE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Estrutura para mensagens saindo (Core 1 -> Core 0 -> Internet)
struct MensagemSaida {
    char topico[32];
    char payload[128];
};

// Estrutura para comandos entrando (Internet -> Core 0 -> Core 1)
struct ComandoEntrada {
    char payload[128]; // JSON bruto para o módulo Comandos parsear
};

// Filas Globais (Inspiradas no seu código antigo)
extern QueueHandle_t filaTX; // Transmissão
extern QueueHandle_t filaRX; // Recepção

class RedeAPI {
public:
    void iniciar();
    bool estaOnline();
    
    // Funções simplificadas para o Core 1 usar
    void enviar(String topico, String json);
    bool temComando(ComandoEntrada& out);

private:
    static void _taskRede(void* pvParameters);
};

extern RedeAPI Rede;

#endif