#ifndef TANK_CONTROLLER_H
#define TANK_CONTROLLER_H

#include <Arduino.h>
#include <queue>
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

// Estrutura de Job (Tarefa)
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
    
    // Comandos externos
    void addJob(float vol, bool encher, String origem);
    void emergencyStop();
    void forceSyncVirtual();

    // Getters de estado
    SystemState getState() { return _currentState; }
    bool isIdle() { return _currentState == STATE_IDLE; }

private:
    volatile SystemState _currentState;
    std::queue<TankJob> _jobQueue;

    float _virtualVolume;    
    float _targetVolume;     
    unsigned long _stateStartTime;
    unsigned long _lastLevelChangeTime;
    float _levelAtPumpStart;

    int _menuLitros;
    bool _menuEncher;
    bool _needsUpdate;

    // Handlers de Estado
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

    // Suporte e Segurança
    void _forceHardwareStop();
    bool _isOperationPossible(TankJob job);
    void _updateStatusLED();
    void _checkMaintenanceConditions();
};

extern TankController controller;

#endif