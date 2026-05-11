#include <ArduinoJson.h>
#include "TankController.h"
#include "TankPhysics.h"
#include "DisplayManager.h"
#include "WiFiService.h"
#include "AuthService.h"
#include "InputManager.h"
#include "HardwareMap.h"

// Instância global para ser usada no main
TankController controller;

// =============================================================================
// CONSTRUTOR E INICIALIZAÇÃO
// =============================================================================

TankController::TankController() : 
    _currentState(STATE_IDLE), 
    _virtualVolume(0.0), 
    _targetVolume(0.0), 
    _stateStartTime(0),
    _lastLevelChangeTime(0),
    _levelAtPumpStart(0.0),
    _menuLitros(1), 
    _menuEncher(true), 
    _needsUpdate(true) {}

void TankController::init() {
    // Inicialização de pinos de saída (Bombas)
    pinMode(PIN_BOMBA_ENCHER, OUTPUT);
    pinMode(PIN_BOMBA_ESVAZ, OUTPUT);
    _forceHardwareStop();

    // Sincronização inicial do volume virtual com o real
    _virtualVolume = tank.getVolume();
}

void TankController::update() {
    // 1. Verificações de Segurança de Hardware
    _checkMaintenanceConditions();

    // 2. Escuta Assíncrona de Comandos Remotos (Nuvem/MQTT)
    _processRemoteCommands();

    // 3. Execução da Máquina de Estados
    _handleStateMachine();

    // 4. Atualização visual de Hardware (LEDs)
    _updateStatusLED();

    // Envia telemetria a cada 1 segundo de forma assíncrona
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
        
        if (error) {
            Serial.println("[Controller] Erro ao parsear JSON do comando MQTT");
            return;
        }

        String comando = doc["comando"] | doc["command"] | ""; 
        comando.toUpperCase();
        
        Serial.println("[Controller] Comando remoto recebido: " + comando);

        if (comando == "ENCHER") {
            addJob(doc["valor"].as<float>(), true, "WEB");
        } 
        else if (comando == "ESVAZIAR") {
            addJob(doc["valor"].as<float>(), false, "WEB");
        } 
        else if (comando == "PARAR") {
            emergencyStop();
        } 
        else if (comando == "RESET") {
            if (_currentState == STATE_ERROR || _currentState == STATE_EMERGENCY) {
                _currentState = STATE_IDLE;
                _needsUpdate = true;
                connectivity.queueLog("SISTEMA_RESETADO_VIA_WEB");
            }
        }
        else if (comando == "ESTADO") {
            forceSyncVirtual();
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
            } else {
                connectivity.queueLog("ERRO_VALORES_CALIBRACAO_INVALIDOS", true);
            }
        }
        else if (comando == "TESTE_BOMBA") {
            bool encher = doc["encher"] | true;
            _forceHardwareStop();
            digitalWrite(encher ? PIN_BOMBA_ENCHER : PIN_BOMBA_ESVAZ, HIGH);
            delay(2000); 
            _forceHardwareStop();
            connectivity.queueLog("TESTE_FISICO_BOMBA_OK");
        }
        // --- FASE 3: SINCRONIZAÇÃO DE USUÁRIOS ---
        else if (comando == "SYNC_USER") {
            String uid = doc["uid"].as<String>();
            String nome = doc["nome"].as<String>();
            bool ativo = doc["ativo"] | true;
            
            if (uid.length() > 0) {
                auth.syncUser(uid, nome, ativo);
                connectivity.queueLog("DB_USUARIO_SINCRONIZADO: " + nome);
            }
        }
        else if (comando == "CLEAR_USERS") {
            auth.clearUsers();
            connectivity.queueLog("DB_USUARIOS_LIMPA");
        }
    }
}

void TankController::addJob(float vol, bool encher, String origem) {
    TankJob newJob = { vol, encher, origem };
    
    if (_isOperationPossible(newJob)) {
        _jobQueue.push(newJob);
        // Atualiza o volume virtual (Soma o que já está na fila + o novo)
        _virtualVolume += (encher ? vol : -vol);
        
        connectivity.queueLog(origem + "_ACEITO: " + String(vol) + "L");
    } else {
        // FASE 2: Feedback Ativo de Rejeição
        // 1. Informa o Dashboard remotamente via MQTT
        connectivity.queueLog("NEGADO: LIMITE EXCEDIDO", true);
        
        // 2. Informa o operador fisicamente no LCD
        _forceHardwareStop();
        display.showStatus("OPERACAO NEGADA", "Limite Excedido");
        delay(2000); // Bloqueio visual de 2s aceitável aqui (a bomba está parada)
        _needsUpdate = true;
    }
}

void TankController::emergencyStop() {
    // Limpa toda a fila de tarefas
    while(!_jobQueue.empty()) _jobQueue.pop();
    
    _forceHardwareStop();
    forceSyncVirtual(); // Sincroniza o virtual com o real
    
    _currentState = STATE_IDLE;
    _needsUpdate = true;
    connectivity.queueLog("STOP_EMERGENCIA_ACIONADO");
}

void TankController::forceSyncVirtual() {
    _virtualVolume = tank.getVolume();
}

// =============================================================================
// MÁQUINA DE ESTADOS (FSM)
// =============================================================================

void TankController::_handleStateMachine() {
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
            delay(1500); 
            _menuLitros = 1; 
            _menuEncher = true; 
            _needsUpdate = true;
            _currentState = STATE_LOCAL_CONFIG_DIR;
        } else {
            display.showStatus("UID: " + auth.getActiveUserID(), auth.getActiveUserName());
            delay(4000); 
            auth.logout(); 
            _needsUpdate = true;
        }
        return;
    }

    // 3. FASE 2: Consciência de Queda de Rede (Offline Warning no LCD)
    if (!connectivity.isConnected()) {
        if (millis() - lastOfflineToggle > 2000) {
            isShowingOffline = !isShowingOffline;
            // Alterna a cada 2s entre o aviso e o nível de água
            if (isShowingOffline) display.showStatus("SISTEMA OFFLINE", "Sem Nuvem");
            else display.showIdle(tank.getVolume());
            lastOfflineToggle = millis();
        }
    } else {
        if (isShowingOffline) { // O Wi-Fi acabou de voltar
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
    if (auth.isValidating()) _needsUpdate = true; 
    
    // 6. Avança para execução se a fila tiver tarefas
    if (!_jobQueue.empty()) _currentState = STATE_VALIDATING;
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
    if (inputs.isIncClicked()) {
        _menuLitros++;
        _needsUpdate = true;
    }
    if (inputs.isDecClicked() && _menuLitros > 1) {
        _menuLitros--;
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
    TankJob currentJob = _jobQueue.front();
    _levelAtPumpStart = tank.getVolume();
    _targetVolume = _levelAtPumpStart + (currentJob.encher ? currentJob.volumeSolicitado : -currentJob.volumeSolicitado);
    
    _stateStartTime = millis();
    _lastLevelChangeTime = millis();
    _needsUpdate = true;
    
    // CORRIGIDO: Envio assíncrono não-bloqueante para o Core 0
    connectivity.queueDigitalTwin("EXECUTANDO", tank.getVolume());
    
    _currentState = STATE_EXECUTING;
}

void TankController::_processExecuting() {
    static float lastExecV = 0;

    // Atualiza o display apenas se houver mudança significativa no nível
    if (_needsUpdate || abs(tank.getVolume() - lastExecV) > 0.05) {
        TankJob job = _jobQueue.front();
        display.showExecuting(tank.getVolume(), _targetVolume, job.encher);
        lastExecV = tank.getVolume();
        _needsUpdate = false;
    }

    // Controle físico das bombas
    TankJob job = _jobQueue.front();
    if (job.encher) {
        digitalWrite(PIN_BOMBA_ENCHER, HIGH);
        digitalWrite(PIN_BOMBA_ESVAZ, LOW);
    } else {
        digitalWrite(PIN_BOMBA_ESVAZ, HIGH);
        digitalWrite(PIN_BOMBA_ENCHER, LOW);
    }

    // Verificação de conclusão
    bool atingiuAlvo = job.encher ? (tank.getVolume() >= _targetVolume) : (tank.getVolume() <= _targetVolume);    

    if (atingiuAlvo) {
        float nivelFinal = tank.getVolume();
        _forceHardwareStop();

        // CORRIGIDO: Registro de Auditoria Assíncrono via IPC Queue (Core 1 -> Core 0)
        connectivity.queueAuditLog(auth.getActiveUserID(), 
                        job.encher ? "ABASTECER" : "DRENAR", 
                        job.volumeSolicitado, 
                        _levelAtPumpStart, 
                        nivelFinal);

        // CORRIGIDO: Atualização assíncrona do Digital Twin
        connectivity.queueDigitalTwin("IDLE", nivelFinal);

        _jobQueue.pop();
        _needsUpdate = true;
        _currentState = STATE_IDLE;
    }

    // Proteção: Timeout da Bomba (Verifica se o nível está mudando)
    if (millis() - _lastLevelChangeTime > BOMBA_TIMEOUT_MS) {
        if (abs(tank.getVolume() - _levelAtPumpStart) < 0.05) { // VOLUME_EPSILON
            _needsUpdate = true;
            _currentState = STATE_ERROR;
            connectivity.queueLog("ERRO: BOMBA TRAVADA");
            
            // CORRIGIDO: Digital Twin assíncrono em caso de erro
            connectivity.queueDigitalTwin("ERRO: BOMBA TRAVADA", tank.getVolume());
        } else {
            _lastLevelChangeTime = millis();
            _levelAtPumpStart = tank.getVolume();
        }
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

bool TankController::_isOperationPossible(TankJob job) {
    float projected = _virtualVolume + (job.encher ? job.volumeSolicitado : -job.volumeSolicitado);
    // Valida contra 0 e contra o volume máximo da calibração atual
    if (projected < 0 || projected > tank.getMaxVolume()) return false; 
    return true;
}

void TankController::_updateStatusLED() {
    if (_currentState == STATE_IDLE) {
        analogWrite(PIN_LED_R, 0); analogWrite(PIN_LED_G, 100); analogWrite(PIN_LED_B, 0);
    } else if (_currentState == STATE_EXECUTING) {
        bool enchendo = _jobQueue.front().encher;
        analogWrite(PIN_LED_R, enchendo ? 0 : 255); 
        analogWrite(PIN_LED_G, 0); 
        analogWrite(PIN_LED_B, enchendo ? 255 : 0);
    } else if (_currentState == STATE_EMERGENCY || _currentState == STATE_ERROR) {
        analogWrite(PIN_LED_R, 255); analogWrite(PIN_LED_G, 0); analogWrite(PIN_LED_B, 0);
    }
}

void TankController::_sendTelemetry() {
    unsigned long agora = millis();
    static float ultimoVolumeEnviado = -100.0;
    static unsigned long ultimoEnvio = 0;
    
    bool deveEnviar = false;

    // Em repouso: Verifica a cada 10s. Envia se variar mais de 0.5L
    if (_currentState == STATE_IDLE) {
        if (agora - ultimoEnvio > 10000) {
            if (abs(tank.getVolume() - ultimoVolumeEnviado) > 0.5) deveEnviar = true;
            ultimoEnvio = agora; // Reseta o timer pra não avaliar toda hora
        }
    } 
    // Em execução: Envia a cada 500ms se variar mais de 0.1L
    else if (_currentState == STATE_EXECUTING) {
        if (agora - ultimoEnvio > 500) {
            if (abs(tank.getVolume() - ultimoVolumeEnviado) > 0.5) deveEnviar = true;
            ultimoEnvio = agora;
        }
    }
    // Mudança de status: Se entrou em erro, emergência, ou comando de estado
    else {
        if (agora - ultimoEnvio > 2000) deveEnviar = true;
    }

    if (deveEnviar) {
        ultimoVolumeEnviado = tank.getVolume();
        ultimoEnvio = agora;
        
        String statusStr;
        switch(_currentState) {
            case STATE_IDLE: statusStr = "IDLE"; break;
            case STATE_EXECUTING: statusStr = "EXECUTANDO"; break;
            case STATE_ERROR: statusStr = "ERRO"; break;
            case STATE_EMERGENCY: statusStr = "EMERGENCIA"; break;
            default: statusStr = "DESCONHECIDO"; break;
        }

        connectivity.queueTelemetria(ultimoVolumeEnviado, statusStr);
    }
}