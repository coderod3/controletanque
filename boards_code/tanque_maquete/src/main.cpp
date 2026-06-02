#include <Arduino.h>
#include "HardwareMap.h"
#include "Config.h"

// APIs de Hardware e Lógica
#include "Sensor.h"
#include "Bombas.h"
#include "Botoes.h"
#include "Tela.h"
#include "ControleNivel.h"
#include "LeitorRFID.h"

// Novos Módulos de Integração
#include "Rede.h"      
#include "Usuarios.h"  
#include "Comandos.h"  
#include "Auditoria.h" 
#include "Parametros.h"

enum EstadoSistema { ESPERANDO_RFID, MENU_AJUSTE, EXECUTANDO, SINCRONIZANDO };
EstadoSistema estadoAtual = ESPERANDO_RFID;
EstadoSistema estadoAnterior = ESPERANDO_RFID; // Para gatilho instantâneo de web

float alvoVol = 0.0;
unsigned long delayTelemetria = 0;
unsigned long ts_animacao_sync = 0; 

// --- VARIÁVEIS GLOBAIS DO SUB-MENU ---
int subMenu = 0;         
int acaoSelecionada = 1; 
float quantidadeL = 0.0; 
unsigned long ts_ultimo_interacao = 0; // Timeout de inatividade

// --- VARIÁVEIS GLOBAIS DE TRACING ---
unsigned long ts_recebido = 0;
unsigned long ts_inicio = 0;
float vol_inicial_tarefa = 0.0;
String ctx_origem = "";
String ctx_usuario = "";
float ctx_lim_encher = 100.0;
float ctx_lim_esvaziar = 100.0;

// =========================================================================
// FUNÇÃO DE ESPELHAMENTO INSTANTÂNEO PARA O SITE (Vercel/Zustand)
// =========================================================================
void espelharEstadoWeb() {
    float v = Sensor.lerLitros();
    String s = "idle";
    
    if (estadoAtual == EXECUTANDO) {
        if (ControleNivel.getAlvo() > v) s = "filling";
        else s = "draining";
    } 
    else if (estadoAtual == MENU_AJUSTE) {
        s = "menu"; // Trava o ControlPanel no site
    } 
    else if (estadoAtual == SINCRONIZANDO) {
        s = "ocupado";
    }
    
    String json = "{\"nivel\":" + String(v, 1) + ",\"estado\":\"" + s + "\"}";
    Rede.enviar(TOPIC_TELEMETRIA, json);
    delayTelemetria = millis(); // Reseta o loop de 2s para evitar spam
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- REVITA OS | INICIALIZANDO ---");
    
    Rede.iniciar();
    Usuarios.iniciar();
    Auditoria.iniciar(); 
    
    Sensor.iniciar();
    Bombas.iniciar();
    Botoes.iniciar();
    Tela.iniciar();
    ControleNivel.iniciar();
    LeitorRFID.iniciar();
    Comandos.iniciar();

    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);

    Tela.atualizar("    REVITA OS    ", " SISTEMA ONLINE ");
    delay(1000);
}

void loop() {
    // ---------------------------------------------------------
    // 0. SINCRONIZAÇÃO E TELEMETRIA (Core 0/1)
    // ---------------------------------------------------------
    Comandos.monitorar();

    if (Comandos.pendenteSync) {
        Comandos.pendenteSync = false; 
        if (estadoAtual == EXECUTANDO) ControleNivel.parar(); 
        
        estadoAtual = SINCRONIZANDO;
        ts_animacao_sync = millis();
        Tela.limpar();
    }

    if (ControleNivel.estaTrabalhando() && estadoAtual != EXECUTANDO && estadoAtual != SINCRONIZANDO) {
        if (ctx_origem == "") {
            ctx_origem = "WEB_DASHBOARD";
            ctx_usuario = "OPERADOR_WEB"; 
            ts_recebido = millis();
        }

        estadoAtual = EXECUTANDO;
        ts_inicio = millis();
        vol_inicial_tarefa = Sensor.lerLitros();
    }

    bool bombaEnchendo = digitalRead(PIN_BOMBA_ENCHER);
    bool bombaEsvaziando = digitalRead(PIN_BOMBA_ESVAZ);
    Sensor.setDirecao(bombaEnchendo, bombaEsvaziando);

    float volAtual = Sensor.lerLitros();
    ComandoBotao btn = Botoes.ler();

    // Renova o tempo de sessão se houver interação física
    if (btn != NENHUM) {
        ts_ultimo_interacao = millis();
    }

    // GATILHO INSTANTÂNEO: Se mudou de estado, avisa a web no mesmo milissegundo!
    if (estadoAtual != estadoAnterior) {
        espelharEstadoWeb();
        estadoAnterior = estadoAtual;
    }

    // TELEMETRIA DE ROTINA: Atualiza a cada 2 segundos
    if (millis() - delayTelemetria > 2000) {
        espelharEstadoWeb();
    }

    // ---------------------------------------------------------
    // 1. MÁQUINA DE ESTADOS (CORE 1)
    // ---------------------------------------------------------
    switch (estadoAtual) {
        
        case SINCRONIZANDO:
            analogWrite(PIN_LED_R, 0);
            analogWrite(PIN_LED_G, 255);
            analogWrite(PIN_LED_B, 255);

            if (millis() - ts_animacao_sync < 1500) {
                Tela.atualizar(" GRAVANDO NVS.. ", " AGUARDE O SYNC ");
            } 
            else if (millis() - ts_animacao_sync < 3000) {
                Tela.atualizar("   PARAMETROS   ", " ATUALIZADOS OK ");
            } 
            else {
                Tela.limpar();
                estadoAtual = ESPERANDO_RFID; 
            }
            break;

        case ESPERANDO_RFID:
            Tela.atualizar("ACESSO RESTRITO ", " PASSE O CARTAO ");
            
            {
                String uidLido = LeitorRFID.lerTag();
                if (uidLido != "") {
                    String nomeUser;
                    if (Usuarios.autenticar(uidLido, nomeUser, ctx_lim_encher, ctx_lim_esvaziar)) {                        
                        ctx_origem = "LOCAL_RFID";
                        ctx_usuario = nomeUser;
                        ts_recebido = millis(); 
                        ts_ultimo_interacao = millis(); // Inicia a sessão de 10s
                        
                        int posEspaco = nomeUser.indexOf(" "); // retorna o índice do primeiro espaço
                        String primeiroNome;

                        if (posEspaco != -1) {
                            // pega do início até o espaço
                            primeiroNome = nomeUser.substring(0, posEspaco);
                        } else {
                            // se não houver espaço, pega o nome inteiro
                            primeiroNome = nomeUser;
                        }
                        
                        String saudacao = "OLA, " + primeiroNome;
                        Tela.atualizar(saudacao, "ACESSO LIBERADO ");
                        Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Acesso local por " + nomeUser + "\"}");
                        delay(1500);
                        
                        estadoAtual = MENU_AJUSTE;
                        subMenu = 0;
                        acaoSelecionada = 1;
                        quantidadeL = 5.0; 

                        Tela.limpar();
                    } else {
                        Tela.atualizar("  TAG INVALIDA  ", uidLido.substring(0, 16));
                        Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Tentativa de acesso negada: " + uidLido + "\", \"tipo\": \"error\"}");
                        delay(1500);
                    }
                }
            }
            break;

        case MENU_AJUSTE:
            // ----------------------------------------------------
            // GESTÃO DE SESSÃO: TIMEOUT DE 10 SEGUNDOS
            // ----------------------------------------------------
            if (millis() - ts_ultimo_interacao > 10000) {
                estadoAtual = ESPERANDO_RFID;
                Tela.limpar();
                Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Sessao encerrada por inatividade (10s).\", \"tipo\": \"warning\"}");
                break; // IMPORTANTE: Pula todo o resto do código e volta pro inicio
            }

            // ----------------------------------------------------
            // ETAPA 1: ESCOLHER A AÇÃO
            // ----------------------------------------------------
            if (subMenu == 0) {
                if (btn == MAIS || btn == MENOS) {
                    acaoSelecionada = (acaoSelecionada == 1) ? -1 : 1;
                }

                if (acaoSelecionada == 1) {
                    Tela.atualizar("> 1. ENCHER     ", "  2. ESVAZIAR   ");
                } else {
                    Tela.atualizar("  1. ENCHER     ", "> 2. ESVAZIAR   ");
                }

                if (btn == CONFIRMA) {
                    subMenu = 1; 
                    quantidadeL = 5.0; 
                    Tela.limpar();
                }
            } 
            // ----------------------------------------------------
            // ETAPA 2: ESCOLHER A QUANTIDADE
            // ----------------------------------------------------
            else if (subMenu == 1) {
                float limiteCota = (acaoSelecionada == 1) ? ctx_lim_encher : ctx_lim_esvaziar;
                float limiteFisico = (acaoSelecionada == 1) ? (Parametros.getTankMaxVolume() - volAtual) : volAtual;
                
                float limiteReal = limiteCota;
                if (limiteFisico < limiteCota) limiteReal = limiteFisico;
                if (limiteReal < 0.0f) limiteReal = 0.0f;

                if (btn == MAIS)  quantidadeL += 5.0;
                if (btn == MENOS) quantidadeL -= 5.0;

                quantidadeL = constrain(quantidadeL, 0.0f, limiteReal);

                String l1 = (acaoSelecionada == 1) ? "ENCHER QUANTO?" : "ESVAZIAR QUANTO?";
                String l2 = String(quantidadeL, 0) + " L (Max:" + String(limiteReal, 0) + ")";
                while(l2.length() < 16) l2 += " "; 
                
                Tela.atualizar(l1, l2.substring(0, 16));

                if (btn == CONFIRMA) {
                    if (quantidadeL > 0) {
                        alvoVol = volAtual + (acaoSelecionada * quantidadeL);
                        ControleNivel.setarAlvo(alvoVol);
                        
                        vol_inicial_tarefa = volAtual;
                        ts_inicio = millis();
                        
                        estadoAtual = EXECUTANDO;
                        Tela.limpar();
                    } else {
                        estadoAtual = ESPERANDO_RFID;
                        Tela.limpar();
                    }
                }
            }
            break;

        case EXECUTANDO:
            ControleNivel.atualizar(volAtual);
            
            {
                float alvoReal = ControleNivel.getAlvo();
                String l1 = "ALVO: " + String(alvoReal, 0) + " L";
                String l2 = "ATUAL : " + String(volAtual, 1) + " L";
                
                while(l1.length() < 16) l1 += " ";
                while(l2.length() < 16) l2 += " ";
                
                Tela.atualizar(l1, l2);
            }

            // O Kill Switch e Fim de Operação juntos
            if (!ControleNivel.estaTrabalhando() || btn == CONFIRMA) {
                
                // Se foi interrompido fisicamente pelo botão (Kill Switch)
                if (btn == CONFIRMA) {
                    Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Operacao interrompida manualmente na placa!\", \"tipo\": \"error\"}");
                }

                ControleNivel.parar();
                estadoAtual = ESPERANDO_RFID; 
                Tela.limpar();
                
                LogOperacao logAudit;
                float alvoReq = ControleNivel.getAlvo();
                
                strncpy(logAudit.tipo_operacao,
                        (alvoReq > vol_inicial_tarefa) ? "ENCHER" : "ESVAZIAR",
                        sizeof(logAudit.tipo_operacao) - 1);
                logAudit.tipo_operacao[sizeof(logAudit.tipo_operacao) - 1] = '\0';

                strncpy(logAudit.usuario_id,
                        ctx_usuario.c_str(),
                        sizeof(logAudit.usuario_id) - 1);
                logAudit.usuario_id[sizeof(logAudit.usuario_id) - 1] = '\0';

                strncpy(logAudit.origem_comando,
                        ctx_origem.c_str(),
                        sizeof(logAudit.origem_comando) - 1);
                logAudit.origem_comando[sizeof(logAudit.origem_comando) - 1] = '\0';

                strncpy(logAudit.status,
                        (btn == CONFIRMA) ? "CANCELADO" : "SUCESSO",
                        sizeof(logAudit.status) - 1);
                logAudit.status[sizeof(logAudit.status) - 1] = '\0';

                logAudit.volume_alvo = alvoReq;
                logAudit.volume_inicial = vol_inicial_tarefa;
                logAudit.volume_final = Sensor.lerLitros();
                
                logAudit.recebido_ms = ts_recebido;
                logAudit.inicio_ms = ts_inicio;
                logAudit.fim_ms = millis();
                
                Auditoria.registrar(logAudit);
                
                ctx_origem = "";
                ctx_usuario = "";
            }
            break;
    }

    // ---------------------------------------------------------
    // 2. FEEDBACK VISUAL (LED RGB PWM)
    // ---------------------------------------------------------
    if (estadoAtual != SINCRONIZANDO) {
        if (bombaEnchendo) {
            analogWrite(PIN_LED_R, 0);
            analogWrite(PIN_LED_G, 255);
            analogWrite(PIN_LED_B, 0);
        } 
        else if (bombaEsvaziando) {
            analogWrite(PIN_LED_R, 255);
            analogWrite(PIN_LED_G, 0);
            analogWrite(PIN_LED_B, 0);
        } 
        else {
            analogWrite(PIN_LED_R, 0);
            analogWrite(PIN_LED_G, 0);
            analogWrite(PIN_LED_B, 100);
        }
    }

    delay(10); 
}