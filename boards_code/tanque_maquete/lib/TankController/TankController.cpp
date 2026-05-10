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
    
    // Lê a fila de rede (retorna instantaneamente se estiver vazia)
    if (connectivity.readPendingCommand(cmd)) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, cmd.payload);
        
        if (error) {
            Serial.println("[Controller] Erro ao parsear JSON do comando MQTT");
            return;
        }

        String comando = doc["comando"] | doc["command"] | ""; // Aceita as duas chaves
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
        // --- PASSO 2: CALIBRAÇÃO DINÂMICA VIA REDE ---
        else if (comando == "CALIBRAR") {
            float maxVol = doc["max_vol"];
            float distVazio = doc["dist_vazio"];
            float distCheio = doc["dist_cheio"];
            
            // Validação de segurança antes de aplicar na memória Flash
            if (maxVol > 0 && distVazio > distCheio) {
                tank.syncConfig(maxVol, distVazio, distCheio); // Grava na Flash e aplica
                connectivity.queueLog("CALIBRACAO_ATUALIZADA");
            } else {
                connectivity.queueLog("ERRO_VALORES_CALIBRACAO_INVALIDOS", true);
            }
        }
        // --- NOVO: BYPASS DE HARDWARE PARA TESTE CRU ---
        else if (comando == "TESTE_BOMBA") {
            bool encher = doc["encher"] | true;
            Serial.println("[Hardware] Teste direto de bomba acionado.");
            
            // Bypass completo: Para tudo, injeta sinal HIGH por 2s e desliga.
            _forceHardwareStop();
            digitalWrite(encher ? PIN_BOMBA_ENCHER : PIN_BOMBA_ESVAZ, HIGH);
            delay(2000); // 2 segundos cravados (seguro para o Watchdog do RTOS)
            _forceHardwareStop();
            
            connectivity.queueLog("TESTE_FISICO_BOMBA_OK");
        }
    }
}

void TankController::addJob(float vol, bool encher, String origem) {
    TankJob newJob = { vol, encher, origem };
    
    // Valida se a operação é fisicamente possível baseado na calibração atual
    if (_isOperationPossible(newJob)) {
        _jobQueue.push(newJob);
        
        // Atualiza o volume virtual (Soma o que já está na fila + o novo)
        _virtualVolume += (encher ? vol : -vol);
        
        connectivity.queueLog(origem + "_ACEITO: " + String(vol) + "L");
    } else {
        connectivity.queueLog(origem + "_NEGADO: LIMITE_EXCEDIDO");
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

    // 1. Feedback visual enquanto espera a resposta da rede
    if (auth.isValidating()) {
        if (!wasValidating) {
            display.showStatus("VALIDANDO TAG", "Aguarde a rede...");
            wasValidating = true;
        }
        auth.update(); // Mantém processando a fila da nuvem
        return; 
    } 
    // 2. Acabou de receber a resposta da Vercel!
    else if (wasValidating) {
        wasValidating = false;
        
        if (auth.isAuthorized()) {
            display.showStatus("ACESSO LIBERADO", auth.getActiveUserName());
            delay(1500); // Mostra o nome do operador antes de pular pro menu
            _menuLitros = 1; 
            _menuEncher = true; 
            _needsUpdate = true;
            _currentState = STATE_LOCAL_CONFIG_DIR;
        } else {
            // MOSTRA O ERRO NA TELA (Ex: "Nao Cadastrada" ou "HTTP Error -1")
            display.showStatus("ACESSO NEGADO", auth.getActiveUserName());
            delay(3000); // Trava a tela por 3 segundos para você conseguir ler
            _needsUpdate = true;
        }
        return;
    }

    // 3. Exibição padrão do tanque em repouso
    if (_needsUpdate) {
        display.showIdle(tank.getVolume());
        _needsUpdate = false;
    }
    
    // 4. Inicia varredura física do cartão
    auth.update();
    if (auth.isValidating()) {
        _needsUpdate = true; // Força a tela a mudar no próximo ciclo
    }
    
    // 5. Se houver tarefas remotas na fila, avança para execução
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
    // Dispara apenas a cada 1000 milissegundos (1 segundo)
    if (millis() - _lastTelemetryTime >= 1000) {
        _lastTelemetryTime = millis();
        
        // Traduz o estado numérico para string
        String statusStr;
        switch(_currentState) {
            case STATE_IDLE: statusStr = "IDLE"; break;
            case STATE_EXECUTING: statusStr = "EXECUTANDO"; break;
            case STATE_ERROR: statusStr = "ERRO"; break;
            case STATE_EMERGENCY: statusStr = "EMERGENCIA"; break;
            default: statusStr = "DESCONHECIDO"; break;
        }

        // Envia para a Fila do MQTT (O Core 0 despacha para a nuvem em background)
        connectivity.queueTelemetria(tank.getVolume(), statusStr);
    }
}