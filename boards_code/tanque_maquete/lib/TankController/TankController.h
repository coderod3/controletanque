#ifndef TANK_CONTROLLER_H
#define TANK_CONTROLLER_H

#include <Arduino.h>
#include <deque> 
#include "HardwareMap.h"

// Definições de Estado do Sistema (FSM)
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

// Estrutura de Job
struct TankJob {
    float volumeSolicitado;
    bool encher;
    String origem;
};

class TankController {
public:
    TankController();
    void init();
    void update();
    
    void addJob(float vol, bool encher, String origem);
    void emergencyStop();

    // Getters
    SystemState getState() const { return _currentState; }
    bool isIdle() const { return _currentState == STATE_IDLE; }

private:
    SystemState _currentState;
    std::deque<TankJob> _jobQueue;

    // Controle Físico
    float _targetVolume = 0.0f;
    float _lastExecDisplayVol = 0.0f;

    // Navegação Local (Menu)
    int _menuLitros = 0;
    bool _menuEncher = true;
    bool _needsUpdate = false;

    // Controle Não-Bloqueante
    unsigned long _waitTimer = 0;
    SystemState _nextStateAfterWait = STATE_IDLE;
    bool _isWaiting = false;

    // Telemetria
    unsigned long _lastTelemetryTime = 0;
    SystemState _lastTelemetryState = STATE_IDLE;
    float _lastTelemetryVol = -100.0f;

    // Métodos Privados
    void _waitAndGo(SystemState nextState, unsigned long ms);

    void _handleStateMachine();
    void _processIdle();
    void _processMaintenance();
    void _processLocalConfigDir();
    void _processLocalConfigVol();
    void _processLocalConfirm();
    void _processValidating();
    void _processExecuting();
    void _processError();
    void _processEmergency();

    void _forceHardwareStop();
    bool _isOperationPossible(TankJob job);
    void _updateStatusLED();
    void _checkMaintenanceConditions();
    void _processRemoteCommands();
    void _sendTelemetry();
};

extern TankController controller;

#endif