#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// SELEÇÃO DE HARDWARE
// =============================================================================
#define USE_ULTRASONIC_SENSOR
// #define USE_LASER_SENSOR 

// =============================================================================
// MAPEAMENTO DE PINOS (ESP32 DevKit V1)
// =============================================================================
#define PIN_SDA          21
#define PIN_SCL          22
#define PIN_TRIGGER      4
#define PIN_ECHO         35 
#define PIN_BOMBA_ENCHER 16
#define PIN_BOMBA_ESVAZ  17
#define PIN_RFID_SS      5
#define PIN_RFID_RST     2
#define PIN_RFID_SCK     18
#define PIN_RFID_MISO    19
#define PIN_RFID_MOSI    23
#define PIN_LED_R        27
#define PIN_LED_G        26
#define PIN_LED_B        25
#define PIN_BTN_INC      13 
#define PIN_BTN_DEC      32 
#define PIN_BTN_CONF     33 
#define PIN_OVERFLOW     14 

// =============================================================================
// PARÂMETROS DO TANQUE (GEOMETRIA)
// =============================================================================
// Parâmetros da Maquete (PET 2L)
/*
#define TANK_HEIGHT_EMPTY 18.0  // 18cm lidos = Vazio
#define TANK_HEIGHT_FULL  2.0   // 2cm lidos = Cheio
#define TANK_MAX_DIST     20.0  // Acima disso = Erro/Manutenção
#define TANK_MAX_VOLUME   100.0 // Nova escala alvo: 100L
#define TANK_BASE_AREA    6.25  // Área calculada para 2L em 16cm de delta
#define TANK_SAFE_MARGIN  0.5
*/

// =============================================================================
// PARÂMETROS DO TANQUE (Escala 1:1 para Emulador)
// =============================================================================
#define TANK_HEIGHT_EMPTY 100.0 // 100cm de distância = 0 Litros
#define TANK_HEIGHT_FULL  0.0   // 0cm de distância = 100 Litros
#define TANK_MAX_DIST     105.0 // Margem para erro de leitura no emulador
#define TANK_MAX_VOLUME   100.0 // Volume máximo para travas de software
#define TANK_BASE_AREA    1.0   // 1cm de variação = 1 Litro
#define TANK_SAFE_MARGIN  0.0

// =============================================================================
// CONFIGURAÇÕES DE REDE (Onde estavam os erros)
// =============================================================================
#define WIFI_SSID         "Wokwi-GUEST"
#define WIFI_PASSWORD     ""

#define MQTT_SERVER       "b5dfbf70844741db84d31800c3bd77a0.s1.eu.hivemq.cloud"
#define MQTT_PORT         8883            // Porta segura para o ESP32
#define MQTT_USER         "esp32_tanque"
#define MQTT_PASS         "Macron@12"
#define MQTT_CLIENT_ID    "nexus_tank_01"

// Tópicos de comunicação
#define TOPIC_TELEMETRIA  "tanque/telemetria"
#define TOPIC_COMANDO     "tanque/comando"

// =============================================================================
// CONSTANTES DE SOFTWARE E TIMEOUTS
// =============================================================================
#define BOMBA_TIMEOUT_MS  5000
#define VOLUME_EPSILON    0.05
#define SENSOR_SAMPLES    10
#define AUTH_SESSION_TIME 30000

#endif // CONFIG_H