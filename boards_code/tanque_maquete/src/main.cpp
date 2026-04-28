#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"
#include <HTTPClient.h>
#include <queue>

// Inclusão dos nossos módulos modulares
#include "TankPhysics.h"
#include "DisplayManager.h"
#include "WiFiService.h"
#include "AuthService.h"
#include "InputManager.h"


// --- Definições de Estado do Sistema (FSM) ---
enum SystemState {
    STATE_IDLE,
    STATE_LOCAL_CONFIG_DIR,
    STATE_LOCAL_CONFIG_VOL,
    STATE_LOCAL_CONFIRM,
    STATE_VALIDATING,
    STATE_EXECUTING,
    STATE_ERROR,
    STATE_EMERGENCY,
    STATE_MAINTENANCE
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

float virtualVolume = 0.0;    
float targetVolume = 0.0;     
unsigned long stateStartTime = 0;
unsigned long lastLevelChangeTime = 0;
float levelAtPumpStart = 0.0;

int menuLitros = 1;
bool menuEncher = true;
bool needsUpdate = true;

// --- Protótipos das Funções Extraídas ---
void handleCommunication();
void checkMaintenanceConditions();
void handleStateMachine();
void processIdle();
void processMaintenance();
void processLocalConfigDir();
void processLocalConfigVol();
void processLocalConfirm();
void processValidating();
void processExecuting();
void processError();
void processEmergency();

// --- Protótipos de Suporte Original ---
void IRAM_ATTR handleOverflowInterrupt();
void forceHardwareStop();
bool isOperationPossible(TankJob job);
void updateStatusLED(SystemState state);
void syncDigitalTwin(String estado);

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    
    pinMode(PIN_BOMBA_ENCHER, OUTPUT);
    pinMode(PIN_BOMBA_ESVAZ, OUTPUT);
    forceHardwareStop();

    pinMode(PIN_OVERFLOW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_OVERFLOW), handleOverflowInterrupt, FALLING);

    tank.init();
    display.init();
    connectivity.init(); 
    auth.init();
    inputs.init();

    tank.update();
    virtualVolume = tank.getVolume();

    display.showStatus("SISTEMA ONLINE", "VIRTUAL: " + String(virtualVolume) + "L");
    delay(2000);
}

// =============================================================================
// LOOP PRINCIPAL (LIMPO)
// =============================================================================
void loop() {
    // 1. Atualização de Sensores e Periféricos
    tank.update();
    inputs.update();

    // 2. Comunicação e Telemetria
    handleCommunication();

    // 3. Verificações de Segurança de Hardware (Manutenção)
    checkMaintenanceConditions();

    // 4. Execução da Máquina de Estados
    handleStateMachine();
}

// =============================================================================
// EXTRAÇÃO DE FUNÇÕES DE LÓGICA DO LOOP
// =============================================================================

void handleCommunication() {
    // 1. Telemetria (A cada 2 segundos)
    static unsigned long lastMqttPub = 0;
    if (millis() - lastMqttPub > 2000) { 
        connectivity.publishTelemetria(tank.getVolume(), auth.getActiveUserName());
        lastMqttPub = millis();
    }

    // 2. Processamento de Comandos Estruturados
    String jsonRaw = connectivity.getPendingCommand();
    if (jsonRaw != "") {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, jsonRaw);
        
        if (error) {
            connectivity.queueLog("ERRO_JSON_REMOTO");
            return;
        }

        String acao = doc["acao"] | "";

        // CASO A: PARADA DE EMERGÊNCIA
        if (acao == "PARAR" || acao == "DESLIGAR") {
            while(!jobQueue.empty()) jobQueue.pop();
            forceHardwareStop();
            virtualVolume = tank.getVolume(); // Sincroniza o virtual com o real
            currentState = STATE_IDLE;
            needsUpdate = true;
            connectivity.queueLog("STOP_REMOTO");
        } 
        
        // CASO B: NOVA TAREFA (Ex: Abastecer 20L)
        else if (acao == "EXECUTAR") {
            float vol = doc["volume"] | 0.0;
            bool encher = doc["encher"] | true;
            
            TankJob remoteJob = { vol, encher, "REMOTO" };
            
            // Valida se a operação é fisicamente possível (0-100L)
            if (isOperationPossible(remoteJob)) {
                jobQueue.push(remoteJob);
                
                // Atualiza o volume virtual (Soma o que já está na fila + o novo)
                virtualVolume += (encher ? vol : -vol);
                
                connectivity.queueLog("REMOTO_ACEITO: " + String(vol) + "L");
                
                // O loop principal (FSM) detectará a jobQueue cheia e iniciará a execução
            } else {
                connectivity.queueLog("REMOTO_NEGADO: LIMITE_EXCEDIDO");
            }
        }
        // Dentro de handleCommunication(), na parte de processamento de comandos:

        else if (acao == "SYNC_CONFIG") {
            // Usamos o pipe "|" para garantir que, se o JSON falhar, 
            // ele use o valor atual do tanque em vez de zero.
            float maxV   = doc["max_volume"] | tank.getMaxVolume();
            float dVazio = doc["dist_vazio"] | tank.getRawDistance(); // ou um valor seguro do Config.h
            float dCheio = doc["dist_cheio"] | 1.0; // Evita divisão por zero no cálculo de volume
            
            tank.syncConfig(maxV, dVazio, dCheio);
            
            connectivity.queueLog("CONFIG_SYNCED_SUCCESS");
            display.showStatus("CALIBRADO", String(maxV) + "L OK");
            virtualVolume = tank.getVolume();
        }
    }
}

void checkMaintenanceConditions() {
    if (tank.getRawDistance() > TANK_MAX_DIST) {
        if (currentState != STATE_MAINTENANCE && currentState != STATE_EMERGENCY) {
            currentState = STATE_MAINTENANCE;
            needsUpdate = true;
        }
    } else if (currentState == STATE_MAINTENANCE) {
        currentState = STATE_IDLE;
        needsUpdate = true;
    }
}

void handleStateMachine() {
    switch (currentState) {
        case STATE_IDLE:             processIdle(); break;
        case STATE_MAINTENANCE:      processMaintenance(); break;
        case STATE_LOCAL_CONFIG_DIR: processLocalConfigDir(); break;
        case STATE_LOCAL_CONFIG_VOL: processLocalConfigVol(); break;
        case STATE_LOCAL_CONFIRM:    processLocalConfirm(); break;
        case STATE_VALIDATING:       processValidating(); break;
        case STATE_EXECUTING:        processExecuting(); break;
        case STATE_ERROR:            processError(); break;
        case STATE_EMERGENCY:        processEmergency(); break;
    }
}

// =============================================================================
// HANDLERS DOS ESTADOS (EXTRATOS DO SWITCH CASE)
// =============================================================================

void processIdle() {
    if (needsUpdate) {
        display.showIdle(tank.getVolume());
        needsUpdate = false;
    }
    if (auth.update()) {
        menuLitros = 1; menuEncher = true; needsUpdate = true;
        currentState = STATE_LOCAL_CONFIG_DIR;
    }
    if (!jobQueue.empty()) currentState = STATE_VALIDATING;
}

void processMaintenance() {
    if (needsUpdate) {
        forceHardwareStop();
        display.showStatus("EM MANUTENCAO", "DIST: " + String(tank.getRawDistance()) + "cm");
        needsUpdate = false;
    }
    if (tank.getRawDistance() <= TANK_MAX_DIST) {
        currentState = STATE_IDLE;
        needsUpdate = true;
    }
}

void processLocalConfigDir() {
    if (needsUpdate) {
        display.showConfigDir(menuEncher);
        needsUpdate = false;
    }
    if (inputs.isIncClicked() || inputs.isDecClicked()) {
        menuEncher = !menuEncher;
        needsUpdate = true;
    }
    if (inputs.isConfClicked()) {
        needsUpdate = true;
        currentState = STATE_LOCAL_CONFIG_VOL;
    }
}

void processLocalConfigVol() {
    if (needsUpdate) {
        display.showConfigVol(menuLitros);
        needsUpdate = false;
    }
    if (inputs.isIncClicked()) {
        menuLitros++;
        needsUpdate = true;
    }
    if (inputs.isDecClicked() && menuLitros > 1) {
        menuLitros--;
        needsUpdate = true;
    }
    if (inputs.isConfClicked()) {
        needsUpdate = true;
        currentState = STATE_LOCAL_CONFIRM;
    }
}

void processLocalConfirm() {
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
}

void processValidating() {
    TankJob currentJob = jobQueue.front();
    levelAtPumpStart = tank.getVolume();
    targetVolume = levelAtPumpStart + (currentJob.encher ? currentJob.volumeSolicitado : -currentJob.volumeSolicitado);
    
    stateStartTime = millis();
    lastLevelChangeTime = millis();
    needsUpdate = true;
    
    // Gêmeo Digital: Avisa o banco ANTES de ligar a bomba
    syncDigitalTwin("EXECUTANDO");
    
    currentState = STATE_EXECUTING;
}

void processExecuting() {
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
        float nivelFinal = tank.getVolume();
        forceHardwareStop();

        // REGISTRO DE AUDITORIA (HARDWARE -> CLOUD)
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin("https://controletanque.vercel.app/api/telemetria/auditoria");
            http.addHeader("Content-Type", "application/json");

            StaticJsonDocument<256> doc;
            doc["rfid_uid"] = auth.getActiveUserID();
            doc["acao"] = jobQueue.front().encher ? "ABASTECER" : "DRENAR";
            doc["volume"] = jobQueue.front().volumeSolicitado;
            doc["valor_anterior"] = levelAtPumpStart;
            doc["valor_atual"] = nivelFinal;
            doc["status"] = "SUCESSO";

            String json;
            serializeJson(doc, json);
            http.POST(json);
            http.end();
        }

        jobQueue.pop();
        connectivity.queueLog("EVENTO: OPERACAO_CONCLUIDA");
        needsUpdate = true;
        currentState = STATE_IDLE;

        // Gêmeo Digital: Avisa o banco que voltou a ficar livre
        syncDigitalTwin("IDLE");
    }

    if (millis() - lastLevelChangeTime > BOMBA_TIMEOUT_MS) {
        if (abs(tank.getVolume() - levelAtPumpStart) < VOLUME_EPSILON) {
            needsUpdate = true;
            currentState = STATE_ERROR;
            connectivity.queueLog("ERRO: BOMBA TRAVADA");

            // Gêmeo Digital: Avisa o banco sobre a falha crítica
            syncDigitalTwin("ERRO: BOMBA TRAVADA");
        } else {
            lastLevelChangeTime = millis();
            levelAtPumpStart = tank.getVolume();
        }
    }
}

void processError() {
    if (needsUpdate) {
        forceHardwareStop();
        display.showErrorMessage("FALHA BOMBA");
        needsUpdate = false;
    }
}

void processEmergency() {
    if (needsUpdate) {
        forceHardwareStop();
        display.showEmergency(); 
        updateStatusLED(STATE_EMERGENCY);
        
        // Gêmeo Digital: Trava de emergência reflete no banco imediatamente
        syncDigitalTwin("EMERGENCIA");
        
        needsUpdate = false;
    }
}

// =============================================================================
// FUNÇÕES DE SUPORTE E SEGURANÇA (ORIGINAIS)
// =============================================================================

void forceHardwareStop() {
    digitalWrite(PIN_BOMBA_ENCHER, LOW);
    digitalWrite(PIN_BOMBA_ESVAZ, LOW);
}

void IRAM_ATTR handleOverflowInterrupt() {
    digitalWrite(16, LOW); 
    digitalWrite(17, LOW);
    currentState = STATE_EMERGENCY;
    needsUpdate = true; 
}

bool isOperationPossible(TankJob job) {
    float projected = virtualVolume + (job.encher ? job.volumeSolicitado : -job.volumeSolicitado);
    if (projected < 0 || projected > TANK_MAX_VOLUME) return false; 
    return true;
}

void updateStatusLED(SystemState state) {
    if (state == STATE_IDLE) {
        analogWrite(PIN_LED_R, 0); analogWrite(PIN_LED_G, 100); analogWrite(PIN_LED_B, 0);
    } else if (state == STATE_EXECUTING) {
        bool enchendo = jobQueue.front().encher;
        analogWrite(PIN_LED_R, 0); analogWrite(PIN_LED_G, 0); analogWrite(PIN_LED_B, enchendo ? 255 : 0);
        if (!enchendo) analogWrite(PIN_LED_R, 255); 
    } else if (state == STATE_EMERGENCY || state == STATE_ERROR) {
        analogWrite(PIN_LED_R, 255); analogWrite(PIN_LED_G, 0); analogWrite(PIN_LED_B, 0);
    }
}

// --- NOVA FUNÇÃO: Sincroniza o estado atual com o banco (Digital Twin) ---
void syncDigitalTwin(String statusOperacional) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("https://controletanque.vercel.app/api/telemetria/status");
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<128> doc;
        doc["status"] = statusOperacional;
        doc["nivel"] = tank.getVolume();

        String json;
        serializeJson(doc, json);
        http.POST(json);
        http.end();
    }
}