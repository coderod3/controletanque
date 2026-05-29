#ifndef PARAMETROS_H
#define PARAMETROS_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

class ParametrosAPI {
private:
    Preferences prefs;

public:
    void iniciar();

    // Setters Múltiplos
    void atualizarDoJson(JsonDocument& doc);
    
    // Handshake
    String obterJsonCompleto();

    // ==========================================
    // GETTERS (Com Fallback para Config.h)
    // ==========================================
    
    // Tanque
    float getTankHeightEmpty();
    float getTankHeightFull();
    float getTankMaxDist();
    float getTankMaxVolume();
    float getTankSafeMargin();

    // Sensor Ultrassônico
    int   getSensorSamples();
    int   getSensorEchoDelayMs();
    float getSensorMaxSlewRate();
    int   getSensorReadInterval();

    // Telemetria
    float getTelemetryIdleDelta();
    int   getTelemetryIdleInterval();
    float getTelemetryExecDelta();
    int   getTelemetryExecInterval();
};

extern ParametrosAPI Parametros;

#endif