#include "Comandos.h"
#include <ArduinoJson.h>
#include "Rede.h"
#include "ControleNivel.h"
#include "Sensor.h"
#include "Usuarios.h"
#include "Parametros.h" // NOVO

ComandosAPI Comandos;

void ComandosAPI::iniciar() {
    Parametros.iniciar(); // Inicia os parâmetros primeiro
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

    // 1. COMANDOS DE OPERAÇÃO
    if (acao == "ENCHER" || acao == "ESVAZIAR") {
        float delta = doc["valor"] | -1.0f;
        if (delta > 0) {
            float volumeAtual = Sensor.lerLitros();
            float alvoFinal = (acao == "ENCHER") ? volumeAtual + delta : volumeAtual - delta;
            
            // Usa as travas diretamente da Memória/Config
            float maxVol = Parametros.getTankMaxVolume();
            if (alvoFinal > maxVol) alvoFinal = maxVol; 
            if (alvoFinal < 0.0f) alvoFinal = 0.0f; 

            ControleNivel.setarAlvo(alvoFinal);
            
            char logMsg[100];
            snprintf(logMsg, sizeof(logMsg), "{\"msg\": \"Operacao %s aceita. Alvo calculado: %.1f L\"}", acao.c_str(), alvoFinal);
            Rede.enviar("tanque/logs", logMsg);
        }
    } 
    else if (acao == "PARAR") {
        ControleNivel.parar();
        Rede.enviar("tanque/logs", "{\"msg\": \"Emergencia: Sistema parado via WEB\"}");
    }
    
    // 2. PARÂMETROS E HANDSHAKE
    else if (acao == "GET_SYNC") {
        Rede.enviar("tanque/telemetria/parametros", Parametros.obterJsonCompleto());
        Rede.enviar("tanque/telemetria/usuarios", Usuarios.obterJsonLista());
    }
    else if (acao == "SET_PARAM") {
        JsonObject data = doc["payload"];
        Parametros.atualizarDoJson(doc); 
        Rede.enviar("tanque/telemetria/parametros", Parametros.obterJsonCompleto());
        Rede.enviar("tanque/logs", "{\"msg\": \"Parametros de engenharia atualizados remotamente\"}");
        pendenteSync = true; // <--- AVISA A MAIN PARA PISCAR A TELA
    }
    
    // 3. GERENCIAMENTO DE USUÁRIOS (CRUD)
    else if (acao == "SYNC_USER") {
        String uid = doc["uid"] | "";
        String nome = doc["nome"] | "Usuario";
        if (uid != "") {
            Usuarios.salvar(uid, nome);
            Rede.enviar("tanque/telemetria/usuarios", Usuarios.obterJsonLista());
            pendenteSync = true; // <--- AVISA A MAIN
        }
    }
    else if (acao == "DEL_USER") {
        String uid = doc["uid"] | "";
        if (uid != "") {
            Usuarios.remover(uid);
            Rede.enviar("tanque/telemetria/usuarios", Usuarios.obterJsonLista());
            pendenteSync = true; // <--- AVISA A MAIN
        }
    }
    else if (acao == "LIMPAR_MEMORIA") {
        Usuarios.limpar();
        Rede.enviar("tanque/telemetria/usuarios", "[]");
        Rede.enviar("tanque/logs", "{\"msg\": \"Memoria de usuarios formatada\"}");
        pendenteSync = true; // <--- AVISA A MAIN
    }

    // 3. GERENCIAMENTO DE USUÁRIOS (CRUD)
    else if (acao == "SYNC_USER") {
        String uid = doc["uid"] | "";
        String nome = doc["nome"] | "Usuario";
        if (uid != "") {
            Usuarios.salvar(uid, nome);
            Rede.enviar("tanque/telemetria/usuarios", Usuarios.obterJsonLista());
        }
    }
    else if (acao == "DEL_USER") {
        String uid = doc["uid"] | "";
        if (uid != "") {
            Usuarios.remover(uid);
            Rede.enviar("tanque/telemetria/usuarios", Usuarios.obterJsonLista());
        }
    }
    else if (acao == "LIMPAR_MEMORIA") {
        Usuarios.limpar();
        Rede.enviar("tanque/telemetria/usuarios", "[]");
        Rede.enviar("tanque/logs", "{\"msg\": \"Memoria de usuarios formatada\"}");
    }
}