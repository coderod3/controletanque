#include <Arduino.h>
//#include "Config.h"
#include "HardwareMap.h" 
#include "InputManager.h"

InputManager::InputManager() {
    // Inicializa as estruturas dos botões
    _btnInc = {PIN_BTN_INC, HIGH, HIGH, 0, false};
    _btnDec = {PIN_BTN_DEC, HIGH, HIGH, 0, false};
    _btnConf = {PIN_BTN_CONF, HIGH, HIGH, 0, false};
}

void InputManager::init() {
    pinMode(_btnInc.pin, INPUT_PULLUP);
    pinMode(_btnDec.pin, INPUT_PULLUP);
    pinMode(_btnConf.pin, INPUT_PULLUP);
}

void InputManager::update() {
    _processButton(_btnInc);
    _processButton(_btnDec);
    _processButton(_btnConf);
}

void InputManager::_processButton(Button &btn) {
    bool reading = digitalRead(btn.pin);

    // Se o estado mudou (devido a ruído ou pressão real)
    if (reading != btn.lastState) {
        btn.lastDebounceTime = millis();
    }

    // Se o tempo passado é maior que o delay, o estado é estável
    if ((millis() - btn.lastDebounceTime) > _debounceDelay) {
        // Se o estado realmente mudou
        if (reading != btn.currentState) {
            btn.currentState = reading;

            // Detecta a borda de descida (PULLUP: HIGH -> LOW significa pressionado)
            if (btn.currentState == LOW) {
                btn.wasPressed = true;
            }
        }
    }

    btn.lastState = reading;
}

// Getters que "consomem" o evento
bool InputManager::isIncClicked() {
    if (_btnInc.wasPressed) {
        _btnInc.wasPressed = false; // Reset da flag após leitura
        return true;
    }
    return false;
}

bool InputManager::isDecClicked() {
    if (_btnDec.wasPressed) {
        _btnDec.wasPressed = false;
        return true;
    }
    return false;
}

bool InputManager::isConfClicked() {
    if (_btnConf.wasPressed) {
        _btnConf.wasPressed = false;
        return true;
    }
    return false;
}

bool InputManager::isIncPressed() {
    // Retorna true se o estado estável atual for LOW (pressionado)
    return (_btnInc.currentState == LOW); 
}

bool InputManager::isDecPressed() {
    return (_btnDec.currentState == LOW);
}

bool InputManager::isConfPressed() {
    return (_btnConf.currentState == LOW);
}

InputManager inputs;