#include "Comandos.h"
#include <ArduinoJson.h>
#include "Rede.h"
#include "ControleNivel.h"
#include "Sensor.h"
#include "Usuarios.h"
#include "Parametros.h" 
#include "Config.h" // Traz os tópicos globais

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
            Rede.enviar(TOPIC_LOGS, logMsg); // VARIÁVEL
        }
    } 
    else if (acao == "PARAR") {
        ControleNivel.parar();
        Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Emergencia: Sistema parado via WEB\"}"); // VARIÁVEL
    }
    
    // 2. PARÂMETROS E HANDSHAKE
    else if (acao == "GET_SYNC") {
        Rede.enviar(TOPIC_PARAMETROS, Parametros.obterJsonCompleto()); // VARIÁVEL
        Rede.enviar(TOPIC_USUARIOS, Usuarios.obterJsonLista()); // VARIÁVEL
    }
    else if (acao == "SET_PARAM") {
        JsonObject data = doc["payload"];
        Parametros.atualizarDoJson(doc); 
        Rede.enviar(TOPIC_PARAMETROS, Parametros.obterJsonCompleto()); // VARIÁVEL
        Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Parametros de engenharia atualizados remotamente\"}"); // VARIÁVEL
        pendenteSync = true; // <--- AVISA A MAIN PARA PISCAR A TELA
    }
    
    // 3. GERENCIAMENTO DE USUÁRIOS (CRUD) - Bloco 1
    else if (acao == "SYNC_USER") {
        String uid = doc["uid"] | "";
        String nome = doc["nome"] | "Usuario";
        if (uid != "") {
            Usuarios.salvar(uid, nome);
            Rede.enviar(TOPIC_USUARIOS, Usuarios.obterJsonLista()); // VARIÁVEL
            pendenteSync = true; // <--- AVISA A MAIN
        }
    }
    else if (acao == "DEL_USER") {
        String uid = doc["uid"] | "";
        if (uid != "") {
            Usuarios.remover(uid);
            Rede.enviar(TOPIC_USUARIOS, Usuarios.obterJsonLista()); // VARIÁVEL
            pendenteSync = true; // <--- AVISA A MAIN
        }
    }
    else if (acao == "LIMPAR_MEMORIA") {
        Usuarios.limpar();
        Rede.enviar(TOPIC_USUARIOS, "[]"); // VARIÁVEL
        Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Memoria de usuarios formatada\"}"); // VARIÁVEL
        pendenteSync = true; // <--- AVISA A MAIN
    }

    // 3. GERENCIAMENTO DE USUÁRIOS (CRUD) - Bloco 2
    else if (acao == "SYNC_USER") {
        String uid = doc["uid"] | "";
        String nome = doc["nome"] | "Usuario";
        if (uid != "") {
            Usuarios.salvar(uid, nome);
            Rede.enviar(TOPIC_USUARIOS, Usuarios.obterJsonLista()); // VARIÁVEL
        }
    }
    else if (acao == "DEL_USER") {
        String uid = doc["uid"] | "";
        if (uid != "") {
            Usuarios.remover(uid);
            Rede.enviar(TOPIC_USUARIOS, Usuarios.obterJsonLista()); // VARIÁVEL
        }
    }
    else if (acao == "LIMPAR_MEMORIA") {
        Usuarios.limpar();
        Rede.enviar(TOPIC_USUARIOS, "[]"); // VARIÁVEL
        Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Memoria de usuarios formatada\"}"); // VARIÁVEL
    }
}