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
#define TANK_HEIGHT_EMPTY 18.0  // 18cm lidos = Vazio
#define TANK_HEIGHT_FULL  2.0   // 2cm lidos = Cheio
#define TANK_MAX_DIST     20.0  // Acima disso = Erro/Manutenção
#define TANK_BASE_AREA    125.0 // Área calculada para 2L em 16cm de delta
#define TANK_SAFE_MARGIN  0.5

// =============================================================================
// CONFIGURAÇÕES DE REDE (Onde estavam os erros)
// =============================================================================
#define WIFI_SSID         "Wokwi-GUEST"
#define WIFI_PASSWORD     ""
#define MQTT_BROKER       "broker.exemplo.com"
#define MQTT_PORT         1883
#define MQTT_CLIENT_ID     "nexus_tank_01"

// =============================================================================
// CONSTANTES DE SOFTWARE E TIMEOUTS
// =============================================================================
#define BOMBA_TIMEOUT_MS  5000
#define VOLUME_EPSILON    0.05
#define SENSOR_SAMPLES    10
#define AUTH_SESSION_TIME 30000

#endif // CONFIG_H