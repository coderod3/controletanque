#ifndef NETWORK_EVENTS_H
#define NETWORK_EVENTS_H

#include <Arduino.h>

// Identificadores de tipos de transações que o Core 1 quer enviar para fora
enum NetworkEventType {
    EVENT_MQTT_TELEMETRIA,
    EVENT_HTTP_DIGITAL_TWIN,
    EVENT_HTTP_AUDITORIA,
    EVENT_MQTT_LOG_INFO,
    EVENT_MQTT_LOG_ERROR,
    EVENT_HTTP_AUTH_REQUEST // <-- NOVO: Pedido de auth
};

// Estrutura de saída genérica (Core 1 -> Core 0)
struct NetworkEvent {
    NetworkEventType type;
    float level;                // Volume ou nível
    char status[24];            // "OPERANDO", "ERRO", "EMERGENCIA"
    char rfid_uid[16];          // UID do cartão lido
    char acao[32];              // Ação executada
    float valor_anterior;       // Histórico de auditoria
    float valor_atual;          // Novo valor pós alteração
    char log_message[64];       // Mensagem de log ou de erro customizada
};

// Estrutura de recepção de comandos (Core 0 -> Core 1)
struct IncomingCommand {
    char payload[128];          // JSON contendo os comandos vindos do Dashboard MQTT
};

// Estrutura de Resposta de Autenticação (Core 0 -> Core 1)
struct AuthResponseEvent {
    bool isAuthorized;
    char userName[32];
};

#endif // NETWORK_EVENTS_H