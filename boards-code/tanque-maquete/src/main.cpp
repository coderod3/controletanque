#include <Arduino.h>
#include "Config.h"
#include <queue>

// Inclusão dos nossos módulos modulares
#include "config.h"
#include "TankPhysics.h"
#include "DisplayManager.h"
#include "WiFiService.h"
#include "AuthService.h"
#include "InputManager.h"

// --- Definições de Estado do Sistema (FSM) ---
// --- Definições de Estado do Sistema (FSM) ---
enum SystemState {
    STATE_IDLE,
    STATE_LOCAL_CONFIG_DIR, // Passo 1: Escolher Encher/Esvaziarz
    STATE_LOCAL_CONFIG_VOL, // Passo 2: Escolher Litros
    STATE_LOCAL_CONFIRM,    // Passo 3: Revisão e OK
    STATE_VALIDATING,
    STATE_EXECUTING,
    STATE_ERROR,
    STATE_EMERGENCY,
    STATE_MAINTENANCE // Novo estado
};

// --- Estrutura de Job (Tarefa) ---
struct TankJob {
    float volumeSolicitado;
    bool encher;
    String origem;
};

// --- Variáveis de Controle Global ---
volatile SystemState currentState = STATE_IDLE;
std::queue<TankJob> jobQueue;

float virtualVolume = 0.0;    // Volume projetado (Real + Fila)
float targetVolume = 0.0;     // Alvo do Job atual
unsigned long stateStartTime = 0;
unsigned long lastLevelChangeTime = 0;
float levelAtPumpStart = 0.0;

// Variáveis de Configuração Local
int menuLitros = 1;
bool menuEncher = true;
bool needsUpdate = true;

// --- Protótipos de Funções de Segurança ---
void IRAM_ATTR handleOverflowInterrupt();
void forceHardwareStop();
bool isOperationPossible(TankJob job);
void updateStatusLED(SystemState state);

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    
    // 1. Inicialização de Hardware Crítico (Bombas começam desligadas)
    pinMode(PIN_BOMBA_ENCHER, OUTPUT);
    pinMode(PIN_BOMBA_ESVAZ, OUTPUT);
    forceHardwareStop();

    // 2. Configuração da Sonda de Transbordo (Interrupção de Hardware)
    pinMode(PIN_OVERFLOW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_OVERFLOW), handleOverflowInterrupt, FALLING);

    // 3. Inicialização dos Módulos
    tank.init();
    display.init();
    connectivity.init(); // Inicia WiFi no Core 0
    auth.init();
    inputs.init();

    // Sincroniza o volume virtual inicial com o real
    tank.update();
    virtualVolume = tank.getVolume();

    display.showStatus("SISTEMA ONLINE", "VIRTUAL: " + String(virtualVolume) + "L");
    delay(2000);
}

// =============================================================================
// LOOP PRINCIPAL
// =============================================================================
void loop() {
    // Atualizações constantes (Independente do Estado)
    tank.update();
    inputs.update();
    
    if (tank.getRawDistance() > TANK_MAX_DIST) {
        if (currentState != STATE_MAINTENANCE && currentState != STATE_EMERGENCY) {
            currentState = STATE_MAINTENANCE;
            needsUpdate = true;
        }
    } else if (currentState == STATE_MAINTENANCE) {
        // Sai da manutenção se o sensor voltar para a faixa de 0-20cm
        currentState = STATE_IDLE;
        needsUpdate = true;
    }

    switch (currentState) {

        case STATE_IDLE:
            if (needsUpdate) {
                display.showIdle(tank.getVolume());
                needsUpdate = false;
            }
            if (auth.update()) {
                menuLitros = 1; menuEncher = true; needsUpdate = true;
                currentState = STATE_LOCAL_CONFIG_DIR;
            }
            if (!jobQueue.empty()) currentState = STATE_VALIDATING;
            break;

        case STATE_MAINTENANCE:
            if (needsUpdate) {
                forceHardwareStop();
                display.showStatus("EM MANUTENCAO", "DIST: " + String(tank.getRawDistance()) + "cm");
                needsUpdate = false;
            }
            // Retorna ao IDLE se o sensor voltar ao normal
            if (tank.getRawDistance() <= TANK_MAX_DIST) {
                currentState = STATE_IDLE;
                needsUpdate = true;
            }
            break;

        case STATE_LOCAL_CONFIRM:
            if (needsUpdate) {
                display.showConfigConfirm(menuLitros, menuEncher);
                needsUpdate = false;
            }
            if (inputs.isConfClicked()) {
                TankJob localJob = {(float)menuLitros, menuEncher, "LOCAL"};
                if (isOperationPossible(localJob)) {
                    jobQueue.push(localJob);
                    virtualVolume += (menuEncher ? menuLitros : -menuLitros);
                    auth.logout(); 
                    currentState = STATE_IDLE;
                    needsUpdate = true;
                } else {
                    display.showErrorMessage("NIVEL IMPOSSIVEL");
                    delay(2000);
                    needsUpdate = true;
                    currentState = STATE_IDLE;
                }
            }
            break;

        case STATE_LOCAL_CONFIG_DIR:
            if (needsUpdate) {
                display.showConfigDir(menuEncher);
                needsUpdate = false;
            }

            if (inputs.isIncClicked() || inputs.isDecClicked()) {
                menuEncher = !menuEncher;
                needsUpdate = true; // Sinaliza que o valor mudou para redesenhar
            }
            
            if (inputs.isConfClicked()) {
                needsUpdate = true;
                currentState = STATE_LOCAL_CONFIG_VOL;
            }
            break;

        case STATE_LOCAL_CONFIG_VOL:
            if (needsUpdate) {
                display.showConfigVol(menuLitros);
                needsUpdate = false;
            }

            if (inputs.isIncClicked()) {
                menuLitros++;
                needsUpdate = true;
            }
            if (inputs.isDecClicked()) {
                if(menuLitros > 1) {
                    menuLitros--;
                    needsUpdate = true;
                }
            }
            
            if (inputs.isConfClicked()) {
                needsUpdate = true;
                currentState = STATE_LOCAL_CONFIRM;
            }
            break;
         
        case STATE_VALIDATING:
            {
                TankJob currentJob = jobQueue.front();
                levelAtPumpStart = tank.getVolume();
                targetVolume = levelAtPumpStart + (currentJob.encher ? currentJob.volumeSolicitado : -currentJob.volumeSolicitado);
                
                stateStartTime = millis();
                lastLevelChangeTime = millis();
                needsUpdate = true;
                currentState = STATE_EXECUTING;
            }
            break;

        case STATE_EXECUTING:
            {
                // Aqui atualizamos a tela sempre que o volume mudar para mostrar o progresso
                static float lastExecV = 0;
                if (needsUpdate || abs(tank.getVolume() - lastExecV) > 0.05) {
                    TankJob job = jobQueue.front();
                    display.showExecuting(tank.getVolume(), targetVolume, job.encher);
                    lastExecV = tank.getVolume();
                    needsUpdate = false;
                }

                TankJob job = jobQueue.front();
                if (job.encher) {
                    digitalWrite(PIN_BOMBA_ENCHER, HIGH);
                    digitalWrite(PIN_BOMBA_ESVAZ, LOW);
                } else {
                    digitalWrite(PIN_BOMBA_ESVAZ, HIGH);
                    digitalWrite(PIN_BOMBA_ENCHER, LOW);
                }

                bool atingiuAlvo = job.encher ? (tank.getVolume() >= targetVolume) : (tank.getVolume() <= targetVolume);
                
                if (atingiuAlvo) {
                    forceHardwareStop();
                    jobQueue.pop();
                    connectivity.queueLog("JOB_OK: " + job.origem);
                    needsUpdate = true;
                    currentState = STATE_IDLE;
                }

                if (millis() - lastLevelChangeTime > BOMBA_TIMEOUT_MS) {
                    if (abs(tank.getVolume() - levelAtPumpStart) < VOLUME_EPSILON) {
                        needsUpdate = true;
                        currentState = STATE_ERROR;
                        connectivity.queueLog("ERRO: BOMBA TRAVADA");
                    } else {
                        lastLevelChangeTime = millis();
                        levelAtPumpStart = tank.getVolume();
                    }
                }
            }
            break;

        case STATE_ERROR:
            if (needsUpdate) {
                forceHardwareStop();
                display.showErrorMessage("FALHA BOMBA");
                needsUpdate = false;
            }
            break;

        case STATE_EMERGENCY:
            if (needsUpdate) {
                forceHardwareStop();
                display.showEmergency(); // Exibe "!!! PERIGO !!! TRANSBORDO DETEC"
                updateStatusLED(STATE_EMERGENCY);
                needsUpdate = false;
            }
            // O sistema fica travado aqui até um Reset físico, como exige a segurança industrial.
            break;
    }
}

// =============================================================================
// FUNÇÕES DE SUPORTE E SEGURANÇA
// =============================================================================

void forceHardwareStop() {
    digitalWrite(PIN_BOMBA_ENCHER, LOW);
    digitalWrite(PIN_BOMBA_ESVAZ, LOW);
}

// Interrupção de altíssima prioridade (executada na IRAM)
void IRAM_ATTR handleOverflowInterrupt() {
    // Desliga pinos via hardware register (mais rápido)
    digitalWrite(16, LOW); 
    digitalWrite(17, LOW);
    currentState = STATE_EMERGENCY;
    needsUpdate = true; 
}

bool isOperationPossible(TankJob job) {
    float projected = virtualVolume + (job.encher ? job.volumeSolicitado : -job.volumeSolicitado);
    if (projected < 0 || projected > 2.05) return false; // Limite real da garrafa 2L
    return true;
}

void updateStatusLED(SystemState state) {
    // Azul = Executando Encher | Vermelho (B) = Executando Esvaziar | Verde = IDLE | Vermelho (R) = Emergência
    if (state == STATE_IDLE) {
        analogWrite(PIN_LED_R, 0); analogWrite(PIN_LED_G, 100); analogWrite(PIN_LED_B, 0);
    } else if (state == STATE_EXECUTING) {
        bool enchendo = jobQueue.front().encher;
        analogWrite(PIN_LED_R, 0); analogWrite(PIN_LED_G, 0); analogWrite(PIN_LED_B, enchendo ? 255 : 0);
        if (!enchendo) analogWrite(PIN_LED_R, 255); // Roxo/Vermelho para esvaziar
    } else if (state == STATE_EMERGENCY || state == STATE_ERROR) {
        analogWrite(PIN_LED_R, 255); analogWrite(PIN_LED_G, 0); analogWrite(PIN_LED_B, 0);
    }
}