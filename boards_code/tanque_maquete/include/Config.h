#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// SELEÇÃO DE HARDWARE
// =============================================================================
#define USE_ULTRASONIC_SENSOR // #define USE_LASER_SENSOR 


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
//#define WIFI_SSID         "Wokwi-GUEST"
//#define WIFI_PASSWORD     ""

#define WIFI_SSID         "rodrigowifi"
#define WIFI_PASSWORD     "3635840331"

#define MQTT_SERVER       "b5dfbf70844741db84d31800c3bd77a0.s1.eu.hivemq.cloud"
#define MQTT_PORT         8883            // Porta segura para o ESP32
#define MQTT_USER         "esp32_tanque"
#define MQTT_PASS         "Macron@12"
#define MQTT_CLIENT_ID    "nexus_tank_01"

// Tópicos de comunicação
#define TOPIC_TELEMETRIA  "tanque/telemetria"
#define TOPIC_COMANDO     "tanque/comando"

// Comente esta linha para mudar para o EMULADOR
// #define AMBIENTE_REAL 

#endif // CONFIG_H