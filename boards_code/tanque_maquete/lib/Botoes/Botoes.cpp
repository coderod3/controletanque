#include "Botoes.h"
#include "HardwareMap.h" // Assume que os pinos PIN_BTN_INC, PIN_BTN_DEC, PIN_BTN_CONF estão aqui

BotoesAPI Botoes;

void BotoesAPI::iniciar() {
    // INPUT_PULLUP: Assume que os botões fecham circuito com o GND.
    pinMode(PIN_BTN_INC, INPUT_PULLUP);
    pinMode(PIN_BTN_DEC, INPUT_PULLUP);
    pinMode(PIN_BTN_CONF, INPUT_PULLUP);
    ultimoDebounce = 0;
}

ComandoBotao BotoesAPI::ler() {
    // Trava de tempo (Debounce): Só aceita um clique a cada 200ms
    if (millis() - ultimoDebounce < 200) return NENHUM; 

    if (digitalRead(PIN_BTN_INC) == LOW) { ultimoDebounce = millis(); return MAIS; }
    if (digitalRead(PIN_BTN_DEC) == LOW) { ultimoDebounce = millis(); return MENOS; }
    if (digitalRead(PIN_BTN_CONF) == LOW) { ultimoDebounce = millis(); return CONFIRMA; } // <-- Mudei OK para CONFIRMA
    return NENHUM;
}