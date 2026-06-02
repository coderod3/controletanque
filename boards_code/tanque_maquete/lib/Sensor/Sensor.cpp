#include "Sensor.h"
#include "HardwareMap.h"
#include "Config.h" 
#include "Parametros.h" // ADICIONADO: Agora o sensor obedece à Web!

SensorAPI Sensor;

void SensorAPI::iniciar() {
    pinMode(PIN_TRIGGER, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    
    // Começa com a distância do tanque VAZIO por segurança (evita iniciar com 100L falso se o cabo soltar)
    ultimaDistancia = Parametros.getTankHeightEmpty(); 
    ultimoTempoLeitura = 0;
    estadoEnchendo = false;
    estadoEsvaziando = false;
}

void SensorAPI::setDirecao(bool enchendo, bool esvaziando) {
    estadoEnchendo = enchendo;
    estadoEsvaziando = esvaziando;
}

float SensorAPI::lerCm() {
    if (millis() - ultimoTempoLeitura < Parametros.getSensorReadInterval()) return ultimaDistancia;
    ultimoTempoLeitura = millis();

    digitalWrite(PIN_TRIGGER, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIGGER, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIGGER, LOW);

    long duracao = pulseIn(PIN_ECHO, HIGH, 30000); 
    if (duracao == 0) return ultimaDistancia; 

    float distanciaLida = (duracao * 0.0343f) / 2.0f;

    if (ultimaDistancia > 0.0) {
        // Efeito Catraca Anti-Ondas
        if (estadoEnchendo && distanciaLida > ultimaDistancia) {
            distanciaLida = ultimaDistancia;
        }
        else if (estadoEsvaziando && distanciaLida < ultimaDistancia) {
            distanciaLida = ultimaDistancia;
        }
        
        // Filtro EMA agora obedece ao número de amostras definidas no painel Web
        float alpha = 2.0f / (Parametros.getSensorSamples() + 1.0f);
        ultimaDistancia = (alpha * distanciaLida) + ((1.0f - alpha) * ultimaDistancia);
    } else {
        ultimaDistancia = distanciaLida;
    }

    return ultimaDistancia;
}

float SensorAPI::lerPorcentagem() {
    float cm = lerCm();
    
    // Puxa as medidas reais configuradas na Web / NVS
    float vazio = Parametros.getTankHeightEmpty();
    float cheio = Parametros.getTankHeightFull();

    if (cm >= vazio) return 0.0f;
    if (cm <= cheio) return 100.0f;
    
    // Mapeamento linear exato de 0 a 100%
    return ((vazio - cm) / (vazio - cheio)) * 100.0f;
}

float SensorAPI::lerLitros() {
    // Se a capacidade máxima no painel web for 100L, 100/100 = 1 (A porcentagem = Litros)
    return lerPorcentagem() * (Parametros.getTankMaxVolume() / 100.0f); 
}