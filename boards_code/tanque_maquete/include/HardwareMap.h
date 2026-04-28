#ifndef HARDWARE_MAP_H
#define HARDWARE_MAP_H

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
// CONSTANTES DE SOFTWARE E TIMEOUTS
// =============================================================================
#define BOMBA_TIMEOUT_MS  5000
#define VOLUME_EPSILON    0.05
#define SENSOR_SAMPLES    10
#define AUTH_SESSION_TIME 30000

#endif