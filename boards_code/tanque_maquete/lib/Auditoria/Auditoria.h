#ifndef AUDITORIA_H
#define AUDITORIA_H

#include <Arduino.h>

struct LogOperacao {
    String tipo_operacao;
    String usuario_id;
    String origem_comando;
    String status;
    float volume_alvo;
    float volume_inicial;
    float volume_final;
    unsigned long recebido_ms;
    unsigned long inicio_ms;
    unsigned long fim_ms;
};

class AuditoriaAPI {
public:
    void iniciar();
    void registrar(LogOperacao log);

private:
    static void taskHTTP(void *pvParameters);
    static void sincronizarNTP();
};

extern AuditoriaAPI Auditoria;

#endif