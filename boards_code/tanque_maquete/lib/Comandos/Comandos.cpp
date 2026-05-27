#include "Comandos.h"
#include <ArduinoJson.h>
#include "Rede.h"
#include "ControleNivel.h"
#include "Sensor.h"
#include "Usuarios.h"

ComandosAPI Comandos;

void ComandosAPI::iniciar() {
    Serial.println("[Comandos] Interprete de mensagens pronto.");
}

void ComandosAPI::monitorar() {
    ComandoEntrada cmd;
    if (Rede.temComando(cmd)) {
        _processar(String(cmd.payload));
    }
}

void ComandosAPI::_processar(String json) {
    JsonDocument doc;
    DeserializationError erro = deserializeJson(doc, json);

    if (erro) {
        Serial.print("[Comandos] Erro no JSON: ");
        Serial.println(erro.c_str());
        return;
    }

    String acao = doc["comando"] | "";
    acao.toUpperCase();

    // 1. COMANDOS DE OPERAÇÃO (Cálculo de Delta Volumétrico)
    if (acao == "ENCHER" || acao == "ESVAZIAR") {
        float delta = doc["valor"] | -1.0f;
        
        if (delta > 0) {
            float volumeAtual = Sensor.lerLitros();
            float alvoFinal = 0.0f;

            if (acao == "ENCHER") {
                alvoFinal = volumeAtual + delta;
                if (alvoFinal > 100.0f) alvoFinal = 100.0f; // Trava de segurança (Tanque Cheio)
            } 
            else if (acao == "ESVAZIAR") {
                alvoFinal = volumeAtual - delta;
                if (alvoFinal < 0.0f) alvoFinal = 0.0f; // Trava de segurança (Tanque Vazio)
            }

            ControleNivel.setarAlvo(alvoFinal);
            
            // Avisa o Dashboard que a conta foi feita e a operação começou
            char logMsg[100];
            snprintf(logMsg, sizeof(logMsg), "{\"msg\": \"Operacao %s aceita. Alvo calculado: %.1f L\"}", acao.c_str(), alvoFinal);
            Rede.enviar("tanque/logs", logMsg);
        }
    } 
    else if (acao == "PARAR") {
        ControleNivel.parar();
        Rede.enviar("tanque/logs", "{\"msg\": \"Emergencia: Sistema parado via WEB\"}");
    }
    
    // 2. COMANDO DE GERENCIAMENTO
    else if (acao == "SYNC_USER") {
        String uid = doc["uid"] | "";
        String nome = doc["nome"] | "Usuario";
        
        if (uid != "") {
            Usuarios.salvar(uid, nome);
            Rede.enviar("tanque/logs", "{\"msg\": \"Usuario sincronizado com sucesso\"}");
        }
    }
    
    // 3. COMANDO DE MANUTENÇÃO
    else if (acao == "LIMPAR_MEMORIA") {
        Usuarios.limpar();
        Rede.enviar("tanque/logs", "{\"msg\": \"Memoria de usuarios limpa\"}");
    }
}