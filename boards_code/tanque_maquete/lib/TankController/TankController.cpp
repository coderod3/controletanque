#include <ArduinoJson.h>
#include "TankController.h"
#include "TankPhysics.h"
#include "DisplayManager.h"
#include "WiFiService.h"
#include "AuthService.h"
#include "InputManager.h"
#include "HardwareMap.h"
#include "Config.h"

// Instância global para ser usada no main
TankController controller;

// =============================================================================
// CONSTRUTOR E INICIALIZAÇÃO
// =============================================================================

TankController::TankController() : 
    _currentState(STATE_IDLE), 
    _targetVolume(0.0f), 
    _lastExecDisplayVol(0.0f),
    _menuLitros(5),       
    _menuEncher(true), 
    _needsUpdate(true),
    _waitTimer(0),        
    _nextStateAfterWait(STATE_IDLE),
    _isWaiting(false),
    _lastTelemetryTime(0),
    _lastTelemetryState(STATE_IDLE),
    _lastTelemetryVol(-100.0f) {}

void TankController::init() {
    // Inicialização de pinos de saída (Bombas)
    pinMode(PIN_BOMBA_ENCHER, OUTPUT);
    pinMode(PIN_BOMBA_ESVAZ, OUTPUT);
    _forceHardwareStop();
    
    // O sistema agora depende 100% da leitura em tempo real do TankPhysics
}

void TankController::update() {
    _checkMaintenanceConditions();
    _processRemoteCommands();
    _handleStateMachine();
    _updateStatusLED();

    // SSOT: Injeção de estado físico real no filtro de hardware
    bool isFilling = (digitalRead(PIN_BOMBA_ENCHER) == HIGH);
    bool isEmptying = (digitalRead(PIN_BOMBA_ESVAZ) == HIGH);
    
    tank.setDirection(isFilling, isEmptying);
    tank.update(); 

    _sendTelemetry();
}

// =============================================================================
// MÉTODOS PÚBLICOS (INTERFACE DE COMANDO)
// =============================================================================

void TankController::_processRemoteCommands() {
    IncomingCommand cmd;
    
    if (connectivity.readPendingCommand(cmd)) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, cmd.payload);
        if (error) return; // Retorno limpo e direto sem travar o log

        String comando = doc["comando"] | doc["command"] | ""; 
        comando.toUpperCase();
        
        if (comando == "ENCHER") addJob(doc["valor"].as<float>(), true, "WEB");
        else if (comando == "ESVAZIAR") addJob(doc["valor"].as<float>(), false, "WEB");
        else if (comando == "PARAR") emergencyStop();
        else if (comando == "RESET") {
            if (_currentState == STATE_ERROR || _currentState == STATE_EMERGENCY) {
                _currentState = STATE_IDLE;
                _needsUpdate = true;
                connectivity.queueLog("SISTEMA_RESETADO_VIA_WEB");
            }
        }
        else if (comando == "ESTADO") {
            // SSOT: Lê sempre e apenas a variável real. Nada de forçar sync virtual.
            String statusStr = (_currentState == STATE_EXECUTING) ? "EXECUTANDO" : 
                               (_currentState == STATE_ERROR) ? "ERRO" : "IDLE";
            connectivity.queueDigitalTwin(statusStr, tank.getVolume());
        }
        else if (comando == "CALIBRAR") {
            float maxVol = doc["max_vol"];
            float distVazio = doc["dist_vazio"];
            float distCheio = doc["dist_cheio"];
            if (maxVol > 0 && distVazio > distCheio) {
                tank.syncConfig(maxVol, distVazio, distCheio); 
                connectivity.queueLog("CALIBRACAO_ATUALIZADA");
            }
        }
        else if (comando == "TESTE_BOMBA") {
            bool encher = doc["encher"] | true;
            _forceHardwareStop();
            digitalWrite(encher ? PIN_BOMBA_ENCHER : PIN_BOMBA_ESVAZ, HIGH);
            delay(2000); // Exceção permitida: rotina de stresse de hardware
            _forceHardwareStop();
        }
        else if (comando == "SYNC_USER") {
            String uid = doc["uid"].as<String>();
            String nome = doc["nome"].as<String>();
            if (uid.length() > 0) auth.syncUser(uid, nome, doc["ativo"] | true);
        }
        else if (comando == "CLEAR_USERS") {
            auth.clearUsers();
        }
    }
}

void TankController::addJob(float vol, bool encher, String origem) {
    // Trava de Sanidade: Impede pacotes MQTT corrompidos ou mal-intencionados
    if (vol <= 0.0f) {
        connectivity.queueLog("ERRO: VOLUME INVALIDO (" + String(vol) + ")");
        return;
    }

    TankJob newJob = { vol, encher, origem };
    
    if (_isOperationPossible(newJob)) {
        _jobQueue.push_back(newJob); // Usamos push_back com std::deque
        connectivity.queueLog(origem + "_ACEITO: " + String(vol) + "L");
    } else {
        connectivity.queueLog("NEGADO: LIMITE EXCEDIDO", true);
        _forceHardwareStop();
        display.showStatus("OPERACAO NEGADA", "Limite Excedido");
        
        // Pausa Não-Bloqueante para a UI (se o sistema estiver parado)
        if (_currentState == STATE_IDLE) {
            _waitAndGo(STATE_IDLE, 2000); 
        } else {
            _needsUpdate = true;
        }
    }
}

bool TankController::_isOperationPossible(TankJob job) {
    // 1. A Verdade Única (Volume Físico Real)
    float projected = tank.getVolume();
    
    // 2. Cálculo "On-The-Fly" do que já está na fila
    for (const auto& pendingJob : _jobQueue) {
        projected += (pendingJob.encher ? pendingJob.volumeSolicitado : -pendingJob.volumeSolicitado);
    }
    
    // 3. Adiciona o volume do novo job que está a ser testado
    projected += (job.encher ? job.volumeSolicitado : -job.volumeSolicitado);
    
    // 4. Valida se o tanque vai transbordar ou secar
    if (projected < 0.0f || projected > tank.getMaxVolume()) return false; 
    
    return true;
}

void TankController::emergencyStop() {
    _jobQueue.clear();
    _forceHardwareStop();
    
    _currentState = STATE_IDLE;
    _needsUpdate = true;
    
    connectivity.queueLog("STOP_EMERGENCIA_ACIONADO");
    connectivity.queueDigitalTwin("IDLE", tank.getVolume());   // Alterado para IDLE
}

// =============================================================================
// MÁQUINA DE ESTADOS (FSM)
// =============================================================================

void TankController::_handleStateMachine() {
    // Interceptor Não-Bloqueante: Segura a transição sem travar a CPU
    if (_isWaiting) {
        if (millis() >= _waitTimer) {
            _isWaiting = false;
            _currentState = _nextStateAfterWait;
            _needsUpdate = true;
        }
        return; // Sai sem executar os estados abaixo
    }

    switch (_currentState) {
        case STATE_IDLE:             _processIdle(); break;
        case STATE_MAINTENANCE:      _processMaintenance(); break;
        case STATE_LOCAL_CONFIG_DIR: _processLocalConfigDir(); break;
        case STATE_LOCAL_CONFIG_VOL: _processLocalConfigVol(); break;
        case STATE_LOCAL_CONFIRM:    _processLocalConfirm(); break;
        case STATE_VALIDATING:       _processValidating(); break;
        case STATE_EXECUTING:        _processExecuting(); break;
        case STATE_ERROR:            _processError(); break;
        case STATE_EMERGENCY:        _processEmergency(); break;
    }
}

void TankController::_processIdle() {
    static bool wasValidating = false;
    static unsigned long lastOfflineToggle = 0;
    static bool isShowingOffline = false;

    // 1. Feedback visual contínuo enquanto espera a nuvem responder o RFID
    if (auth.isValidating()) {
        if (!wasValidating) {
            display.showStatus("VALIDANDO TAG", "Aguarde a rede...");
            wasValidating = true;
        }
        auth.update(); 
        return; 
    } 
    // 2. Acabou de receber a resposta da Vercel
    else if (wasValidating) {
        wasValidating = false;
        
        if (auth.isAuthorized()) {
            display.showStatus("ACESSO LIBERADO", auth.getActiveUserName());
            _menuLitros = 5;      // SSOT: Garante que o menu começa em 5%
            _menuEncher = true; 
            _waitAndGo(STATE_LOCAL_CONFIG_DIR, 1500); // UI livre de delays!
        } else {
            display.showStatus("UID: " + auth.getActiveUserID(), auth.getActiveUserName());
            auth.logout(); 
            _waitAndGo(STATE_IDLE, 4000);             // UI livre de delays!
        }
        return;
    }

    // 3. Consciência de Queda de Rede (Offline Warning no LCD)
    if (!connectivity.isConnected()) {
        if (millis() - lastOfflineToggle > 2000) {
            isShowingOffline = !isShowingOffline;
            if (isShowingOffline) display.showStatus("SISTEMA OFFLINE", "Sem Nuvem");
            else display.showIdle(tank.getVolume());
            lastOfflineToggle = millis();
        }
    } else {
        if (isShowingOffline) { 
            isShowingOffline = false;
            _needsUpdate = true;
        }
    }

    // 4. Exibição padrão do tanque em repouso
    if (_needsUpdate && !isShowingOffline) {
        display.showIdle(tank.getVolume());
        _needsUpdate = false;
    }
    
    // 5. Inicia varredura física do cartão
    auth.update();

    if (auth.isValidating()) {
        _needsUpdate = true; 
    }
    
    // 6. Avança para execução se a fila tiver tarefas
    if (!_jobQueue.empty()) {
        _currentState = STATE_VALIDATING;
    }
}

void TankController::_processMaintenance() {
    if (_needsUpdate) {
        _forceHardwareStop();
        display.showStatus("EM MANUTENCAO", "DIST: " + String(tank.getRawDistance()) + "cm");
        _needsUpdate = false;
    }
    // Sai da manutenção se o sensor voltar ao range válido
    if (tank.getRawDistance() <= TANK_MAX_DIST) {
        _currentState = STATE_IDLE;
        _needsUpdate = true;
    }
}

void TankController::_processLocalConfigDir() {
    if (_needsUpdate) {
        display.showConfigDir(_menuEncher);
        _needsUpdate = false;
    }
    if (inputs.isIncClicked() || inputs.isDecClicked()) {
        _menuEncher = !_menuEncher;
        _needsUpdate = true;
    }
    if (inputs.isConfClicked()) {
        _needsUpdate = true;
        _currentState = STATE_LOCAL_CONFIG_VOL;
    }
}

void TankController::_processLocalConfigVol() {
    if (_needsUpdate) {
        display.showConfigVol(_menuLitros);
        _needsUpdate = false;
    }
    
    // Incremento Ágil (Passos de 5%)
    if (inputs.isIncClicked() && _menuLitros <= 95) {
        _menuLitros += 5; 
        _needsUpdate = true;
    }
    
    // Limite mínimo de 5%
    if (inputs.isDecClicked() && _menuLitros >= 10) { 
        _menuLitros -= 5;
        _needsUpdate = true;
    }
    
    if (inputs.isConfClicked()) {
        _needsUpdate = true;
        _currentState = STATE_LOCAL_CONFIRM;
    }
}

void TankController::_processLocalConfirm() {
    if (_needsUpdate) {
        display.showConfigConfirm(_menuLitros, _menuEncher);
        _needsUpdate = false;
    }
    if (inputs.isConfClicked()) {
        addJob((float)_menuLitros, _menuEncher, "LOCAL");
        auth.logout(); 
        _currentState = STATE_IDLE;
        _needsUpdate = true;
    }
}

void TankController::_processValidating() {
    // Proteção básica: se não há tarefa por algum erro, volta pro repouso
    if (_jobQueue.empty()) { 
        _currentState = STATE_IDLE;
        return;
    }

    const auto& currentJob = _jobQueue.front();
    
    // SSOT Perfeita: Calcula o alvo com base na água física exata deste milissegundo
    _targetVolume = tank.getVolume() + (currentJob.encher ? currentJob.volumeSolicitado : -currentJob.volumeSolicitado);
    
    _needsUpdate = true;
    
    connectivity.queueDigitalTwin("EXECUTANDO", tank.getVolume());
    _currentState = STATE_EXECUTING;
}

void TankController::_processExecuting() {
    if (_jobQueue.empty()) {
        _forceHardwareStop();
        _currentState = STATE_IDLE;
        return;
    }

    const TankJob job = _jobQueue.front();

    // Atualização do LCD
    if (_needsUpdate || fabs(tank.getVolume() - _lastExecDisplayVol) >= 0.5f) {
        display.showExecuting(tank.getVolume(), _targetVolume, job.encher);
        _lastExecDisplayVol = tank.getVolume();
        _needsUpdate = false;
    }

    // Aciona bombas
    digitalWrite(PIN_BOMBA_ENCHER, job.encher ? HIGH : LOW);
    digitalWrite(PIN_BOMBA_ESVAZ,  job.encher ? LOW : HIGH);

    // Verifica conclusão
    bool atingiuAlvo = job.encher ? 
        (tank.getVolume() >= _targetVolume) : 
        (tank.getVolume() <= _targetVolume);

    if (atingiuAlvo) {
        float nivelFinal = tank.getVolume();
        _forceHardwareStop();

        float startVol = _targetVolume - (job.encher ? job.volumeSolicitado : -job.volumeSolicitado);

        connectivity.queueAuditLog(auth.getActiveUserID(), 
                        job.encher ? "ABASTECER" : "DRENAR", 
                        job.volumeSolicitado, startVol, nivelFinal);

        connectivity.queueDigitalTwin("IDLE", nivelFinal);

        _jobQueue.pop_front();
        _needsUpdate = true;
        _currentState = STATE_IDLE;
    }
}

void TankController::_processError() {
    if (_needsUpdate) {
        _forceHardwareStop();
        display.showErrorMessage("FALHA BOMBA");
        _needsUpdate = false;
    }
}

void TankController::_processEmergency() {
    if (_needsUpdate) {
        _forceHardwareStop();
        display.showEmergency(); 
        
        // CORRIGIDO: Digital Twin assíncrono em caso de emergência
        connectivity.queueDigitalTwin("EMERGENCIA", tank.getVolume());
        _needsUpdate = false;
    }
}

// =============================================================================
// SUPORTE, SEGURANÇA E AUXILIARES
// =============================================================================

void TankController::_forceHardwareStop() {
    digitalWrite(PIN_BOMBA_ENCHER, LOW);
    digitalWrite(PIN_BOMBA_ESVAZ, LOW);
}

void TankController::_checkMaintenanceConditions() {
    if (tank.getRawDistance() > TANK_MAX_DIST) {
        if (_currentState != STATE_MAINTENANCE && _currentState != STATE_EMERGENCY) {
            _currentState = STATE_MAINTENANCE;
            _needsUpdate = true;
        }
    } else if (_currentState == STATE_MAINTENANCE) {
        _currentState = STATE_IDLE;
        _needsUpdate = true;
    }
}

void TankController::_updateStatusLED() {
    if (_currentState == STATE_IDLE) {
        // Verde suave para repouso
        analogWrite(PIN_LED_R, 0); 
        analogWrite(PIN_LED_G, 100); 
        analogWrite(PIN_LED_B, 0);
    } 
    else if (_currentState == STATE_EXECUTING) {
        // Proteção Crítica: Só acessa a fila se ela não estiver vazia
        if (!_jobQueue.empty()) {
            bool enchendo = _jobQueue.front().encher;
            // Azul para encher, Vermelho/Roxo para esvaziar
            analogWrite(PIN_LED_R, enchendo ? 0 : 255); 
            analogWrite(PIN_LED_G, 0); 
            analogWrite(PIN_LED_B, enchendo ? 255 : 0);
        }
    } 
    else if (_currentState == STATE_EMERGENCY || _currentState == STATE_ERROR) {
        // Vermelho brilhante para falhas
        analogWrite(PIN_LED_R, 255); 
        analogWrite(PIN_LED_G, 0); 
        analogWrite(PIN_LED_B, 0);
    }
}

void TankController::_sendTelemetry() {
    unsigned long agora = millis();
    bool deveEnviar = false;

    // Detecta mudança de estado instantaneamente
    if (_currentState != _lastTelemetryState) deveEnviar = true;

    // Avaliação em repouso
    if (_currentState == STATE_IDLE) {
        if (agora - _lastTelemetryTime > TELEMETRY_IDLE_INTERVAL_MS) {
            if (fabs(tank.getVolume() - _lastTelemetryVol) >= TELEMETRY_IDLE_DELTA_L) deveEnviar = true;
            _lastTelemetryTime = agora; 
        }
    } 
    // Avaliação em execução
    else if (_currentState == STATE_EXECUTING) {
        if (agora - _lastTelemetryTime > TELEMETRY_EXEC_INTERVAL_MS) {
            if (fabs(tank.getVolume() - _lastTelemetryVol) >= TELEMETRY_EXEC_DELTA_L) deveEnviar = true;
            _lastTelemetryTime = agora;
        }
    }
    // Mudança de status crítico
    else {
        if (agora - _lastTelemetryTime > 2000) deveEnviar = true;
    }

    if (deveEnviar) {
        // Atualiza as variáveis da classe (OOP)
        _lastTelemetryVol = tank.getVolume();
        _lastTelemetryState = _currentState;
        _lastTelemetryTime = agora;
        
        String statusStr;
        switch(_currentState) {
            case STATE_IDLE: statusStr = "IDLE"; break;
            case STATE_EXECUTING: statusStr = "EXECUTANDO"; break;
            case STATE_ERROR: statusStr = "ERRO"; break;
            case STATE_EMERGENCY: statusStr = "EMERGENCIA"; break;
            default: statusStr = "DESCONHECIDO"; break;
        }

        connectivity.queueTelemetria(_lastTelemetryVol, statusStr);
    }
}

void TankController::_waitAndGo(SystemState nextState, unsigned long ms) {
    _waitTimer = millis() + ms;
    _nextStateAfterWait = nextState;
    _isWaiting = true;
}

