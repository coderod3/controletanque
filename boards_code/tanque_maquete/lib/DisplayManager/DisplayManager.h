#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

class DisplayManager {
public:
    DisplayManager();
    
    void init();
    
    // Telas de Contexto
    void showIdle(float volume);
    void showAuthWaiting();
    
    // --- NOVAS TELAS DO WIZARD DE CONFIGURAÇÃO ---
    void showConfigDir(bool enchendo);
    void showConfigVol(int litros);
    void showConfigConfirm(int litros, bool enchendo);
    // ---------------------------------------------

    void showExecuting(float atual, float alvo, bool enchendo);
    void showErrorMessage(String msg);
    void showEmergency();

    // Utilitário para mensagens rápidas
    void showStatus(String line1, String line2 = "");

private:
    LiquidCrystal_I2C _lcd;
    // Variáveis para evitar refresh desnecessário (flicker)
    float _lastVolume;
    int _lastState;
    // Adicione estes dois:
    String _lastLine1; 
    String _lastLine2;
};

extern DisplayManager display;

#endif