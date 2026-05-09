#include <Arduino.h>
#include "Config.h"
#include "HardwareMap.h"
#include "WiFiService.h"
#include "DisplayManager.h"
#include "TankPhysics.h"
#include "InputManager.h"
#include "AuthService.h"
#include "TankController.h"

// =============================================================================
// INTERRUPÇÃO DE HARDWARE (SAFETY-FIRST)
// =============================================================================
// ISR de baixíssima latência. Se a boia de overflow bater, corta as bombas
// instantaneamente no nível de interrupção do processador.
void IRAM_ATTR handleOverflowInterrupt() {
    controller.emergencyStop();
}

// =============================================================================
// SETUP DO SISTEMA
// =============================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n--- [NEXUS OS] INICIALIZANDO FIRMWARE INTEGRAL ---");

    // 1. Inicializa Rede e Serviços de Fundo (Core 0)
    connectivity.init();

    // 2. Inicializa Módulos de Hardware Físico (Core 1)
    tank.init();        // VL53L1X / Ultrassónico
    display.init();     // Tela OLED
    auth.init();        // RFID Reader
    inputs.init();      // Botões de comando com Debounce
    controller.init();  // Máquina de estados (FSM) e saídas digitais (Bombas)

    // 3. Configuração de Segurança por Interrupção (Transbordo)
    pinMode(PIN_OVERFLOW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_OVERFLOW), handleOverflowInterrupt, FALLING);

    display.showStatus("NEXUS OS", "SISTEMA ONLINE");
    Serial.println("[System] Setup concluído. Core 1 livre para loop de controle.");
}

// =============================================================================
// LOOP PRINCIPAL (CORE 1 - EXECUÇÃO FÍSICA DETERMINÍSTICA)
// =============================================================================
void loop() {
    // 1. Atualiza leituras físicas passivas e médias móveis
    tank.update();

    // 2. Atualiza estado de clique/pressão dos botões físicos
    inputs.update();

    // 3. Executa a máquina de estados (FSM), processos remotos/locais e telemetria
    // NOTA: O RFID é lido internamente por aqui (dentro do estado IDLE via auth.update())
    controller.update();

    // 4. Delay obrigatório de 10ms para alimentar o Watchdog do FreeRTOS
    delay(10);
}