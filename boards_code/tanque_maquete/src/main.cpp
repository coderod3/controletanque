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

float alvoVol = 0.0;
unsigned long delayTelemetria = 0;
unsigned long ts_animacao_sync = 0; 

// --- VARIÁVEIS GLOBAIS DO SUB-MENU ---
int subMenu = 0;         // 0: Escolher Ação, 1: Escolher Quantidade
int acaoSelecionada = 1; // 1: Encher, -1: Esvaziar
float quantidadeL = 0.0; // Quantidade de Litros a adicionar/remover

// --- VARIÁVEIS GLOBAIS DE TRACING ---
unsigned long ts_recebido = 0;
unsigned long ts_inicio = 0;
float vol_inicial_tarefa = 0.0;
String ctx_origem = "";
String ctx_usuario = "";
float ctx_lim_encher = 100.0;
float ctx_lim_esvaziar = 100.0;

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- NEXUS OS | INICIALIZANDO ---");
    
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

    Tela.atualizar("    NEXUS OS    ", " SISTEMA ONLINE ");
    delay(1000);
}

void loop() {
    // ---------------------------------------------------------
    // 0. SINCRONIZAÇÃO E TELEMETRIA
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

    if (millis() - delayTelemetria > 2000) {
        String statusStr = "idle";
        if (estadoAtual == EXECUTANDO) {
            if (bombaEnchendo) statusStr = "filling";
            else if (bombaEsvaziando) statusStr = "draining";
        } 
        else if (estadoAtual == SINCRONIZANDO) {
            statusStr = "ocupado"; 
        }
        
        String json = "{\"nivel\":" + String(volAtual, 1) + ",\"estado\":\"" + statusStr + "\"}";
        Rede.enviar(TOPIC_TELEMETRIA, json);
        delayTelemetria = millis();
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
                    // Recebe as cotas offline na hora da leitura!
                    if (Usuarios.autenticar(uidLido, nomeUser, ctx_lim_encher, ctx_lim_esvaziar)) {                        
                        ctx_origem = "LOCAL_RFID";
                        ctx_usuario = nomeUser;
                        ts_recebido = millis(); 

                        String saudacao = "OLA, " + nomeUser.substring(0, 11);
                        Tela.atualizar(saudacao, "ACESSO LIBERADO ");
                        Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Acesso local por " + nomeUser + "\"}");
                        delay(1500);
                        
                        // Prepara as variáveis para o Menu Interativo
                        estadoAtual = MENU_AJUSTE;
                        subMenu = 0;
                        acaoSelecionada = 1;
                        quantidadeL = 5.0; // Inicia sugerindo 5L

                        Tela.limpar();
                    } else {
                        Tela.atualizar("  TAG INVALIDA  ", uidLido.substring(0, 16));
                        Rede.enviar(TOPIC_LOGS, "{\"msg\": \"Tentativa de acesso negada: " + uidLido + "\"}");
                        delay(1500);
                    }
                }
            }
            break;

        case MENU_AJUSTE:
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
                    subMenu = 1; // Avança para a Etapa 2
                    quantidadeL = 5.0; 
                    Tela.limpar();
                }
            } 
            // ----------------------------------------------------
            // ETAPA 2: ESCOLHER A QUANTIDADE (COM TRAVAS)
            // ----------------------------------------------------
            else if (subMenu == 1) {
                // 1. Calcula qual é a cota aplicável a esta ação
                float limiteCota = (acaoSelecionada == 1) ? ctx_lim_encher : ctx_lim_esvaziar;
                
                // 2. Calcula qual é o limite físico do tanque no momento
                float limiteFisico = (acaoSelecionada == 1) ? (Parametros.getTankMaxVolume() - volAtual) : volAtual;
                
                // 3. A restrição Real é SEMPRE o menor dos dois!
                float limiteReal = limiteCota;
                if (limiteFisico < limiteCota) limiteReal = limiteFisico;
                if (limiteReal < 0.0f) limiteReal = 0.0f;

                // 4. Interação do usuário
                if (btn == MAIS)  quantidadeL += 5.0;
                if (btn == MENOS) quantidadeL -= 5.0;

                // 5. TRAVA RIGOROSA (Impede passar do teto ou cair abaixo de zero)
                quantidadeL = constrain(quantidadeL, 0.0f, limiteReal);

                // 6. Atualização visual no display
                String l1 = (acaoSelecionada == 1) ? "ENCHER QUANTO?" : "ESVAZIAR QUANTO?";
                String l2 = String(quantidadeL, 0) + " L (Max:" + String(limiteReal, 0) + ")";
                while(l2.length() < 16) l2 += " "; // Padding para limpar artefatos
                
                Tela.atualizar(l1, l2.substring(0, 16));

                // 7. Confirmação Final e Início da Bomba
                if (btn == CONFIRMA) {
                    if (quantidadeL > 0) {
                        alvoVol = volAtual + (acaoSelecionada * quantidadeL);
                        ControleNivel.setarAlvo(alvoVol);
                        
                        vol_inicial_tarefa = volAtual;
                        ts_inicio = millis();
                        
                        estadoAtual = EXECUTANDO;
                        Tela.limpar();
                    } else {
                        // Se o usuário confirmar com "0 Litros", ele desiste e volta
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
                String l2 = "ATU : " + String(volAtual, 1) + " L";
                
                while(l1.length() < 16) l1 += " ";
                while(l2.length() < 16) l2 += " ";
                
                Tela.atualizar(l1, l2);
            }

            if (!ControleNivel.estaTrabalhando() || btn == CONFIRMA) {
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