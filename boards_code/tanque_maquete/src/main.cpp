#include <Arduino.h>
#include "DisplayManager.h"
#include "TankPhysics.h"
#include "InputManager.h"
#include "RFIDReader.h" 
#include "WiFiService.h" // Essencial para manter o OTA ativo em background
#include "Config.h"
#include "HardwareMap.h" 

// Variáveis de controle
unsigned long timerBotao = 0;
String msgBotao = "";
unsigned long timerRFID = 0;
String lastUID = "";

void setup() {
    Serial.begin(115200);
    
    // Inicialização da Rede e OTA (Core 0)
    connectivity.init();

    // Inicialização dos módulos base (Core 1)
    display.init();   
    tank.init();     
    inputs.init();   
    rfidReader.init(); 

    // Configuração dos pinos de saída
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    pinMode(PIN_BOMBA_ENCHER, OUTPUT);
    pinMode(PIN_BOMBA_ESVAZ, OUTPUT);
    
    // Configuração do sensor de overflow / emergência
    pinMode(PIN_OVERFLOW, INPUT_PULLUP); 

    // Garante bombas desligadas no início
    digitalWrite(PIN_BOMBA_ENCHER, LOW);
    digitalWrite(PIN_BOMBA_ESVAZ, LOW);

    Serial.println("--- NEXUS: TESTE INTEGRAL DE HARDWARE ATIVO ---");
}

void loop() {
    // 1. Atualização dos sensores e dispositivos
    tank.update();
    inputs.update();
    
    // 2. Leitura do Botão de Emergência (Lógica Inversa: PULLUP)
    bool emergencia = (digitalRead(PIN_OVERFLOW) == LOW);

    // 3. Controle das Bombas (Substituindo os leds dos botões)
    if (emergencia) {
        // Trava de segurança: Desliga tudo imediatamente
        digitalWrite(PIN_BOMBA_ENCHER, LOW);
        digitalWrite(PIN_BOMBA_ESVAZ, LOW);
    } else {
        // Lógica de acionamento manual por pressão do botão
        digitalWrite(PIN_BOMBA_ENCHER, inputs.isIncPressed() ? HIGH : LOW);
        digitalWrite(PIN_BOMBA_ESVAZ, inputs.isDecPressed() ? HIGH : LOW);
    }

    // 4. Lógica de Ciclo Automático do LED RGB (Cicla a cada 1.5 segundos)
    int cicloLED = (millis() / 1500) % 3;
    if (emergencia) {
        // Alerta visual de emergência: Pisca Vermelho rápido
        digitalWrite(PIN_LED_R, (millis() % 200 < 100) ? HIGH : LOW);
        digitalWrite(PIN_LED_G, LOW);
        digitalWrite(PIN_LED_B, LOW);
    } else {
        // Ciclo automático suave das cores
        digitalWrite(PIN_LED_R, (cicloLED == 0) ? HIGH : LOW);
        digitalWrite(PIN_LED_G, (cicloLED == 1) ? HIGH : LOW);
        digitalWrite(PIN_LED_B, (cicloLED == 2) ? HIGH : LOW);
    }

    // 5. Leitura do RFID
    String currentUID = rfidReader.readTag();
    if (currentUID != "") {
        lastUID = currentUID;
        timerRFID = millis();
    }

    // 6. Captura de Cliques dos botões
    if (inputs.isIncClicked())  { msgBotao = "ENCHER (B1)";    timerBotao = millis(); }
    if (inputs.isDecClicked())  { msgBotao = "ESVAZIAR (B2)";  timerBotao = millis(); }
    if (inputs.isConfClicked()) { msgBotao = "CONFIRMAR (B3)"; timerBotao = millis(); }

    // 7. Lógica de Exibição do Display por Prioridade

    // PRIORIDADE 0: Emergência (Overflow ativo)
    if (emergencia) {
        display.showStatus("!!! PERIGO !!!", "EMERGENCIA ATIVA");
    }
    // PRIORIDADE 1: Segurando botões (Status da Bomba física no display)
    else if (inputs.isIncPressed()) {
        display.showStatus("LIGADO MANUALLY", "BOMBA ENCHENDO"); 
    } 
    else if (inputs.isDecPressed()) {
        display.showStatus("LIGADO MANUALLY", "BOMBA ESVAZIANDO");
    } 
    else if (inputs.isConfPressed()) {
        display.showStatus("SEGURANDO OK", "PIN BTN CONFIRM");
    }
    // PRIORIDADE 2: RFID ativo (Exibe por 4 segundos)
    else if (millis() - timerRFID < 4000 && timerRFID != 0) {
        display.showStatus("TAG DETECTADA:", lastUID);
    }
    // PRIORIDADE 3: Mensagem de clique (Exibe por 1 segundo)
    else if (millis() - timerBotao < 1000 && timerBotao != 0) {
        display.showStatus("CLICADO:", msgBotao);
    }
    // PRIORIDADE 4: Monitoramento em tempo real do nível e distância
    else {
        timerBotao = 0;
        display.showStatus(
            "VOL: " + String(tank.getVolume(), 1) + "L",
            "DIST: " + String(tank.getRawDistance(), 1) + "cm"
        );
    }

    delay(20); 
}