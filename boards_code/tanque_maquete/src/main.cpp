#include <Arduino.h>

// --- DEFINE OS PINOS DE SAÍDA (LIGADOS AO NANO) ---
#define PINO_ESP_ENCHER 16  // Envia 3.3V para o pino D5 do Nano
#define PINO_ESP_ESVAZ  17  // Envia 3.3V para o pino D6 do Nano

// --- DEFINE OS PINOS DOS BOTÕES ---
#define BTN_ENCHER 13       // Botão 1 - Aciona Bomba de Encher
#define BTN_ESVAZ  32       // Botão 2 - Aciona Bomba de Esvaziar
#define BTN_AMBOS  33       // Botão 3 - Aciona AMBAS as bombas

// --- CONFIGURAÇÃO DA LÓGICA DE INSTALAÇÃO DOS BOTÕES ---
// true  = Botão ligado direto no GND (Ativo em LOW - Usa resistores internos do ESP32)
// false = Botão ligado no 3.3V com resistor pull-down externo (Ativo em HIGH)
const bool USAR_PULLUP_INTERNO = true; 

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- INICIANDO TESTE: ESP32 -> NANO -> MOSFET -> BOMBAS ---");

  // Configura os pinos de sinal para o Nano como saídas
  pinMode(PINO_ESP_ENCHER, OUTPUT);
  pinMode(PINO_ESP_ESVAZ, OUTPUT);

  // Garante que as bombas comecem desligadas
  digitalWrite(PINO_ESP_ENCHER, LOW);
  digitalWrite(PINO_ESP_ESVAZ, LOW);

  // Configuração dos pinos de entrada dos botões
  if (USAR_PULLUP_INTERNO) {
    pinMode(BTN_ENCHER, INPUT_PULLUP);
    pinMode(BTN_ESVAZ, INPUT_PULLUP);
    pinMode(BTN_AMBOS, INPUT_PULLUP);
    Serial.println("Botões configurados em modo INPUT_PULLUP (Ativos em nível LOW/GND).");
  } else {
    pinMode(BTN_ENCHER, INPUT);
    pinMode(BTN_ESVAZ, INPUT);
    pinMode(BTN_AMBOS, INPUT);
    Serial.println("Botões configurados em modo INPUT comum (Ativos em nível HIGH/3.3V).");
  }
}

void loop() {
  bool btnEncherPressionado = false;
  bool btnEsvazPressionado = false;
  bool btnAmbosPressionado = false;

  // Realiza a leitura com base na lógica física definida
  if (USAR_PULLUP_INTERNO) {
    // Com INPUT_PULLUP, apertar o botão fecha curto com o GND, retornando LOW
    btnEncherPressionado = (digitalRead(BTN_ENCHER) == LOW);
    btnEsvazPressionado  = (digitalRead(BTN_ESVAZ) == LOW);
    btnAmbosPressionado  = (digitalRead(BTN_AMBOS) == LOW);
  } else {
    // Com pull-down físico externo, apertar o botão joga 3.3V no pino, retornando HIGH
    btnEncherPressionado = (digitalRead(BTN_ENCHER) == HIGH);
    btnEsvazPressionado  = (digitalRead(BTN_ESVAZ) == HIGH);
    btnAmbosPressionado  = (digitalRead(BTN_AMBOS) == HIGH);
  }

  // --- LÓGICA DE ACIONAMENTO ---

  if (btnAmbosPressionado) {
    // Botão 3: Prioridade máxima. Liga ambas as bombas enquanto segurar
    digitalWrite(PINO_ESP_ENCHER, HIGH);
    digitalWrite(PINO_ESP_ESVAZ, HIGH);
    Serial.println("[COMANDO] Botão Ambos pressionado: Ativando tudo.");
  } 
  else {
    // Se o botão de acionamento duplo estiver solto, controla individualmente

    // Controle Bomba de Encher (Botão 1)
    if (btnEncherPressionado) {
      digitalWrite(PINO_ESP_ENCHER, HIGH);
      Serial.println("[COMANDO] Botão 1 pressionado: Ligando Bomba Encher (Pino 16).");
    } else {
      digitalWrite(PINO_ESP_ENCHER, LOW);
    }

    // Controle Bomba de Esvaziar (Botão 2)
    if (btnEsvazPressionado) {
      digitalWrite(PINO_ESP_ESVAZ, HIGH);
      Serial.println("[COMANDO] Botão 2 pressionado: Ligando Bomba Esvaziar (Pino 17).");
    } else {
      digitalWrite(PINO_ESP_ESVAZ, LOW);
    }
  }

  // Pequeno delay para evitar bouncing mecânico dos botões e poluição no serial
  delay(30); 
}