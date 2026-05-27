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
#include "Rede.h"      // Orquestrador do Core 0
#include "Usuarios.h"  // Banco de dados local na Flash
#include "Comandos.h"  // Intérprete de mensagens MQTT

// Estados do Sistema
enum EstadoSistema { ESPERANDO_RFID, MENU_AJUSTE, EXECUTANDO };
EstadoSistema estadoAtual = ESPERANDO_RFID;

float alvoVol = 0.0;
bool menuIniciado = false;
unsigned long delayTelemetria = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- NEXUS OS | INICIALIZANDO ---");
    
    // 1. Inicializa Infraestrutura de Rede (Dispara o Core 0)
    Rede.iniciar();
    Usuarios.iniciar();
    
    // 2. Inicializa Hardware e Lógica (Core 1)
    Sensor.iniciar();
    Bombas.iniciar();
    Botoes.iniciar();
    Tela.iniciar();
    ControleNivel.iniciar();
    LeitorRFID.iniciar();
    Comandos.iniciar();

    // 3. Configuração de Periféricos
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

    // Intercepta comandos vindos da Web para forçar o estado de Execução
    if (ControleNivel.estaTrabalhando() && estadoAtual != EXECUTANDO) {
        estadoAtual = EXECUTANDO;
        menuIniciado = false; 
    }
    
    bool bombaEnchendo = digitalRead(PIN_BOMBA_ENCHER);
    bool bombaEsvaziando = digitalRead(PIN_BOMBA_ESVAZ);
    Sensor.setDirecao(bombaEnchendo, bombaEsvaziando);

    float volAtual = Sensor.lerLitros();
    ComandoBotao btn = Botoes.ler();

    // Envia telemetria para o site a cada 2 segundos
    if (millis() - delayTelemetria > 2000) {
        
        // CORREÇÃO DO BUG 1: Dicionário traduzido para o React ("filling", "draining", "idle")
        String statusStr = "idle";
        if (estadoAtual == EXECUTANDO) {
            if (bombaEnchendo) statusStr = "filling";
            else if (bombaEsvaziando) statusStr = "draining";
        }
        
        String json = "{\"nivel\":" + String(volAtual, 1) + ",\"estado\":\"" + statusStr + "\"}";
        Rede.enviar(TOPIC_TELEMETRIA, json);
        delayTelemetria = millis();
    }

    // ---------------------------------------------------------
    // 1. MÁQUINA DE ESTADOS (CORE 1)
    // ---------------------------------------------------------
    switch (estadoAtual) {
        
        case ESPERANDO_RFID:
            Tela.atualizar("ACESSO RESTRITO ", " PASSE O CARTAO ");
            
            {
                String uidLido = LeitorRFID.lerTag();
                if (uidLido != "") {
                    String nomeUser;
                    if (Usuarios.autenticar(uidLido, nomeUser)) {
                        String saudacao = "OLA, " + nomeUser.substring(0, 11);
                        Tela.atualizar(saudacao, "ACESSO LIBERADO ");
                        Rede.enviar("tanque/logs", "{\"msg\": \"Acesso local por " + nomeUser + "\"}");
                        delay(1500);
                        estadoAtual = MENU_AJUSTE;
                        menuIniciado = false;
                        Tela.limpar();
                    } else {
                        Tela.atualizar("  TAG INVALIDA  ", uidLido.substring(0, 16));
                        Rede.enviar("tanque/logs", "{\"msg\": \"Tentativa de acesso negada: " + uidLido + "\"}");
                        delay(1500);
                    }
                }
            }
            break;

        case MENU_AJUSTE:
            if (!menuIniciado) {
                alvoVol = round(volAtual / 5.0) * 5.0; 
                alvoVol = constrain(alvoVol, 0.0, 100.0);
                menuIniciado = true;
            }

            {
                String linhaAlvo = "ALVO: " + String(alvoVol, 0) + " L";
                while(linhaAlvo.length() < 16) linhaAlvo += " ";
                Tela.atualizar("  AJUSTAR ALVO  ", linhaAlvo);
            }
            
            if (btn == MAIS)  alvoVol += 5.0;
            if (btn == MENOS) alvoVol -= 5.0;
            alvoVol = constrain(alvoVol, 0.0, 100.0);

            if (btn == CONFIRMA) { 
                ControleNivel.setarAlvo(alvoVol);
                estadoAtual = EXECUTANDO;
                Tela.limpar();
            }
            break;

        case EXECUTANDO:
            ControleNivel.atualizar(volAtual);
            
            {
                // CORREÇÃO DO BUG 3: Lê o alvo real da API (ControleNivel) em vez da variável local
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
            }
            break;
    }

    // ---------------------------------------------------------
    // 2. FEEDBACK VISUAL (LED RGB PWM)
    // ---------------------------------------------------------
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

    delay(10); 
}