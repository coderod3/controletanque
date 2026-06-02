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
#define TANK_SAFE_MARGIN  0.5
*/

// =============================================================================
// PARÂMETROS DO TANQUE (Escala 1:1 para Emulador)
// =============================================================================
#define TANK_HEIGHT_EMPTY 100.0 // 100cm de distância = 0 Litros
#define TANK_HEIGHT_FULL  2.0   // 0cm de distância = 100 Litros
#define TANK_MAX_DIST     18.0 // Margem para erro de leitura no emulador
#define TANK_MAX_VOLUME   20.0 // Volume máximo para travas de software
#define TANK_SAFE_MARGIN  0.5

// --- CONFIGURAÇÕES DO SENSOR ULTRASSÔNICO ---
#define SENSOR_SAMPLES 8                  // Quantidade de leituras para extrair a mediana
#define SENSOR_ECHO_DELAY_MS 40           // Tempo para o eco dissipar (evita o sensor "pular")
#define SENSOR_MAX_SLEW_RATE_CM_S 6.0f     // Salto máximo permitido por segundo (ignora erros brutais)
#define SENSOR_READ_INTERVAL       120     // ms entre leituras completas

// =============================================================================
// PARÂMETROS DE TELEMETRIA E SEGURANÇA FÍSICA
// =============================================================================
#define TELEMETRY_IDLE_DELTA_L       1.5    // Variação em % (ou L) exigida para envio em repouso
#define TELEMETRY_IDLE_INTERVAL_MS   10000  // Frequência de avaliação em repouso (10s)
#define TELEMETRY_EXEC_DELTA_L       0.8    // Variação em % (ou L) exigida para envio com bomba ligada
#define TELEMETRY_EXEC_INTERVAL_MS   400    // Frequência de avaliação com bomba ligada (0.5s)
// #define PUMP_STALL_DELTA_L           0.05   // Variação mínima esperada antes de declarar "FALHA BOMBA"


// =============================================================================
// CONFIGURAÇÕES DE REDE (Onde estavam os erros)
// =============================================================================
#define WIFI_SSID         "Wokwi-GUEST"
#define WIFI_PASSWORD     ""

//#define WIFI_SSID         "rodrigowifi"
//#define WIFI_PASSWORD     "3635840331"

// configs mqtt modo seguro criptografado

#define MQTT_SERVER       "b5dfbf70844741db84d31800c3bd77a0.s1.eu.hivemq.cloud"
#define MQTT_PORT         8883            // Porta segura para o ESP32
#define MQTT_USER         "esp32_tanque"
#define MQTT_PASS         "Macron@12"
#define MQTT_CLIENT_ID    "nexus_tank_01"

// #define MQTT_SERVER       "b5dfbf70844741db84d31800c3bd77a0.s1.eu.hivemq.cloud"
// #define MQTT_PORT         8883            // Porta segura com criptografia
// #define MQTT_USER         "esp32_tanque"
// #define MQTT_PASS         "Macron@12"
// #define MQTT_CLIENT_ID    "nexus_hw_002_fisico" // ID único

// =============================================================================
// CONFIGURAÇÕES DE REDE (MODO APRESENTAÇÃO - SEM SSL)
// =============================================================================
// #define MQTT_SERVER       "15.197.214.98" // Broker Público Rápido
// #define MQTT_PORT         1883                // Porta TCP Crua (Sem Handshake SSL)
// #define MQTT_USER         ""                  
// #define MQTT_PASS         ""                  
// #define MQTT_CLIENT_ID    "nexus_hw_002_pres" // ID único

// Tópicos de comunicação seguro
//#define TOPIC_TELEMETRIA  "tanque/telemetria"
//#define TOPIC_COMANDO     "tanque/comando"

// topicos apresentacao
//#define TOPIC_TELEMETRIA  "nx_tanque_8f7d6c5b/telemetria"
//#define TOPIC_COMANDO     "nx_tanque_8f7d6c5b/comando"

#define MQTT_PREFIX "nx_tanque_8f7d6c5b" // SUA ÚNICA VARIÁVEL GLOBAL

// O C++ vai concatenar as strings automaticamente durante a compilação
#define TOPIC_TELEMETRIA  MQTT_PREFIX "/telemetria"
#define TOPIC_COMANDO     MQTT_PREFIX "/comando"
#define TOPIC_LOGS        MQTT_PREFIX "/logs"
#define TOPIC_PARAMETROS  MQTT_PREFIX "/telemetria/parametros"
#define TOPIC_USUARIOS    MQTT_PREFIX "/telemetria/usuarios"
#define TOPIC_STATUS      MQTT_PREFIX "/status"

// Comente esta linha para mudar para o EMULADOR
// #define AMBIENTE_REAL 

// =============================================================================
// AUDITORIA E REST API
// =============================================================================
#define NTP_SERVER         "pool.ntp.org"
#define API_AUDITORIA_URL  "https://controletanque.vercel.app/api/auditoria" // Mude para seu domínio depois


#endif // CONFIG_H