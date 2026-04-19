#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Estrutura para encapsular o estado de cada botão
struct Button {
    uint8_t pin;
    bool lastState;
    bool currentState;
    unsigned long lastDebounceTime;
    bool wasPressed; // Flag de evento "clicado"
};

class InputManager {
public:
    InputManager();
    
    void init();
    
    // Processa o debounce de todos os botões (deve ser chamado no loop)
    void update();
    
    // Getters de eventos (retornam true apenas uma vez por clique)
    bool isIncClicked();
    bool isDecClicked();
    bool isConfClicked();

private:
    Button _btnInc;
    Button _btnDec;
    Button _btnConf;

    void _processButton(Button &btn);
    const unsigned long _debounceDelay = 50; // 50ms é o padrão industrial
};

extern InputManager inputs;

#endif