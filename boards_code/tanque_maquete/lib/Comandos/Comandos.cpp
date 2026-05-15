#include "Comandos.h"
#include <ArduinoJson.h>
#include "Rede.h"
#include "ControleNivel.h"
#include "Usuarios.h"

ComandosAPI Comandos;

void ComandosAPI::iniciar() {
    Serial.println("[Comandos] Interprete de mensagens pronto.");
}

void ComandosAPI::monitorar() {
    ComandoEntrada cmd;
    // Se a API Rede tiver um comando na fila, nós o pegamos
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

    // 1. COMANDOS DE OPERAÇÃO (Direto para o ControleNivel)
    if (acao == "ENCHER" || acao == "ESVAZIAR") {
        float valor = doc["valor"] | -1.0f;
        if (valor >= 0) {
            ControleNivel.setarAlvo(valor);
            Rede.enviar("tanque/logs", "{\"msg\": \"Operacao " + acao + " iniciada via WEB\"}");
        }
    } 
    else if (acao == "PARAR") {
        ControleNivel.parar();
        Rede.enviar("tanque/logs", "{\"msg\": \"Sistema parado via WEB\"}");
    }
    
    // 2. COMANDO DE GERENCIAMENTO (Sincronização de Usuários)
    else if (acao == "SYNC_USER") {
        String uid = doc["uid"] | "";
        String nome = doc["nome"] | "Usuario";
        
        if (uid != "") {
            Usuarios.salvar(uid, nome);
            Serial.println("[Comandos] Novo usuario sincronizado: " + nome);
            Rede.enviar("tanque/logs", "{\"msg\": \"Usuario " + nome + " salvo na memoria local\"}");
        }
    }
    
    // 3. COMANDO DE MANUTENÇÃO
    else if (acao == "LIMPAR_MEMORIA") {
        Usuarios.limpar();
        Rede.enviar("tanque/logs", "{\"msg\": \"Memoria de usuarios limpa\"}");
    }
}