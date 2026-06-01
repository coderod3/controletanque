#include "Parametros.h"
#include "Config.h" // Puxa os hardcoded defaults

ParametrosAPI Parametros;

void ParametrosAPI::iniciar() {
    prefs.begin("parametros", false); // Abre a Flash
    Serial.println("[Parametros] Memoria NVS carregada.");
}

// Retorna tudo para o Site
String ParametrosAPI::obterJsonCompleto() {
    JsonDocument doc;
    
    doc["TANK_HEIGHT_EMPTY"] = getTankHeightEmpty();
    doc["TANK_HEIGHT_FULL"] = getTankHeightFull();
    doc["TANK_MAX_DIST"] = getTankMaxDist();
    doc["TANK_MAX_VOLUME"] = getTankMaxVolume();
    doc["TANK_SAFE_MARGIN"] = getTankSafeMargin();
    
    doc["SENSOR_SAMPLES"] = getSensorSamples();
    doc["SENSOR_ECHO_DELAY_MS"] = getSensorEchoDelayMs();
    doc["SENSOR_MAX_SLEW_RATE_CM_S"] = getSensorMaxSlewRate();
    doc["SENSOR_READ_INTERVAL"] = getSensorReadInterval();
    
    doc["TELEMETRY_IDLE_DELTA_L"] = getTelemetryIdleDelta();
    doc["TELEMETRY_IDLE_INTERVAL_MS"] = getTelemetryIdleInterval();
    doc["TELEMETRY_EXEC_DELTA_L"] = getTelemetryExecDelta();
    doc["TELEMETRY_EXEC_INTERVAL_MS"] = getTelemetryExecInterval();

    String saida;
    serializeJson(doc, saida);
    return saida;
}

// Recebe do Site e salva na Flash
// Recebe do Site e salva na Flash
void ParametrosAPI::atualizarDoJson(JsonDocument& doc) {
    if (doc["TANK_HEIGHT_EMPTY"].is<float>()) prefs.putFloat("tk_h_emp", doc["TANK_HEIGHT_EMPTY"]);
    if (doc["TANK_HEIGHT_FULL"].is<float>())  prefs.putFloat("tk_h_ful", doc["TANK_HEIGHT_FULL"]);
    if (doc["TANK_MAX_DIST"].is<float>())     prefs.putFloat("tk_m_dst", doc["TANK_MAX_DIST"]);
    if (doc["TANK_MAX_VOLUME"].is<float>())   prefs.putFloat("tk_m_vol", doc["TANK_MAX_VOLUME"]);
    if (doc["TANK_SAFE_MARGIN"].is<float>())  prefs.putFloat("tk_s_mar", doc["TANK_SAFE_MARGIN"]);

    if (doc["SENSOR_SAMPLES"].is<int>())            prefs.putInt("sn_samp", doc["SENSOR_SAMPLES"]);
    if (doc["SENSOR_ECHO_DELAY_MS"].is<int>())      prefs.putInt("sn_echo", doc["SENSOR_ECHO_DELAY_MS"]);
    if (doc["SENSOR_MAX_SLEW_RATE_CM_S"].is<float>()) prefs.putFloat("sn_slew", doc["SENSOR_MAX_SLEW_RATE_CM_S"]);
    if (doc["SENSOR_READ_INTERVAL"].is<int>())      prefs.putInt("sn_int",  doc["SENSOR_READ_INTERVAL"]);

    if (doc["TELEMETRY_IDLE_DELTA_L"].is<float>())     prefs.putFloat("tl_id_del", doc["TELEMETRY_IDLE_DELTA_L"]);
    if (doc["TELEMETRY_IDLE_INTERVAL_MS"].is<int>())   prefs.putInt("tl_id_int", doc["TELEMETRY_IDLE_INTERVAL_MS"]);
    if (doc["TELEMETRY_EXEC_DELTA_L"].is<float>())     prefs.putFloat("tl_ex_del", doc["TELEMETRY_EXEC_DELTA_L"]);
    if (doc["TELEMETRY_EXEC_INTERVAL_MS"].is<int>())   prefs.putInt("tl_ex_int", doc["TELEMETRY_EXEC_INTERVAL_MS"]);
}


// --- GETTERS ---
float ParametrosAPI::getTankHeightEmpty() { return prefs.getFloat("tk_h_emp", TANK_HEIGHT_EMPTY); }
float ParametrosAPI::getTankHeightFull()  { return prefs.getFloat("tk_h_ful", TANK_HEIGHT_FULL); }
float ParametrosAPI::getTankMaxDist()     { return prefs.getFloat("tk_m_dst", TANK_MAX_DIST); }
float ParametrosAPI::getTankMaxVolume()   { return prefs.getFloat("tk_m_vol", TANK_MAX_VOLUME); }
float ParametrosAPI::getTankSafeMargin()  { return prefs.getFloat("tk_s_mar", TANK_SAFE_MARGIN); }

int   ParametrosAPI::getSensorSamples()       { return prefs.getInt("sn_samp", SENSOR_SAMPLES); }
int   ParametrosAPI::getSensorEchoDelayMs()   { return prefs.getInt("sn_echo", SENSOR_ECHO_DELAY_MS); }
float ParametrosAPI::getSensorMaxSlewRate()   { return prefs.getFloat("sn_slew", SENSOR_MAX_SLEW_RATE_CM_S); }
int   ParametrosAPI::getSensorReadInterval()  { return prefs.getInt("sn_int", SENSOR_READ_INTERVAL); }

float ParametrosAPI::getTelemetryIdleDelta()     { return prefs.getFloat("tl_id_del", TELEMETRY_IDLE_DELTA_L); }
int   ParametrosAPI::getTelemetryIdleInterval()  { return prefs.getInt("tl_id_int", TELEMETRY_IDLE_INTERVAL_MS); }
float ParametrosAPI::getTelemetryExecDelta()     { return prefs.getFloat("tl_ex_del", TELEMETRY_EXEC_DELTA_L); }
int   ParametrosAPI::getTelemetryExecInterval()  { return prefs.getInt("tl_ex_int", TELEMETRY_EXEC_INTERVAL_MS); }