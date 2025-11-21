#include <Wire.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

// -------------------- Constantes --------------------
const int pinoTrigger = 6;
const int pinoEcho = 7;
const int pinoBombaEncher = 9;
const int pinoBombaEsvaziar = 10;
const int pinoBotaoEncher = 4;
const int pinoBotaoEsvaziar = 5;
const int pinoEmergencia = 8;
const int pinoPause = 12;

const float alturaTanque = 20;   // cm
const float toleranciaNivel = 2; // cm

const unsigned long intervaloEnvio = 1000; // ms
const unsigned long debounceDelay = 50;    // ms
const bool DEBUG = true; // flag para prints

// -------------------- Objetos --------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial nodeSerial(2, 3);

// -------------------- Variáveis globais --------------------
float nivelAtual = 0;
float nivelDesejado = 5;
bool nivelAtingido = false;
bool pausado = false;

unsigned long tempoAnterior = 0;

// -------------------- Setup --------------------
void setup() {
  Serial.begin(9600);
  nodeSerial.begin(9600);

  pinMode(pinoTrigger, OUTPUT);
  pinMode(pinoEcho, INPUT);
  pinMode(pinoBombaEncher, OUTPUT);
  pinMode(pinoBombaEsvaziar, OUTPUT);
  pinMode(pinoBotaoEncher, INPUT);
  pinMode(pinoBotaoEsvaziar, INPUT);
  pinMode(pinoEmergencia, INPUT);
  pinMode(pinoPause, INPUT);

  atualizarNivel();
  nivelDesejado = nivelAtual;

  lcd.init();
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print("Tanque 100L");
  atualizarTela();
}

// -------------------- Loop --------------------
void loop() {
  tratarBotaoPause();
  tratarEmergencia();
  tratarBotoesManuais();
  tratarControleAutomatico();
  tratarComunicacaoNode();
  tratarEntradaSerialDebug(); // opcional para testes
  atualizarTelaSeNecessario();
}

// -------------------- Funções --------------------

// Sensor + atualização centralizada
float medirDistancia() {
  digitalWrite(pinoTrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pinoTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinoTrigger, LOW);

  long duracao = pulseIn(pinoEcho, HIGH, 30000); // timeout 30ms
  if (duracao == 0) return nivelAtual; // se não houve eco, mantém último valor
  return duracao * 0.034 / 2; // cm
}

void atualizarNivel() {
  nivelAtual = medirDistancia();
}

// Display
void atualizarTela() {
  atualizarNivel();
  lcd.setCursor(8, 1);
  lcd.print("      ");
  lcd.setCursor(8, 1);
  lcd.print(String((nivelAtual / alturaTanque) * 100, 2));
}

void atualizarTelaSeNecessario() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnterior >= intervaloEnvio) {
    if (DEBUG) {
      Serial.print((nivelAtingido) ? "Aceitavel " : "FORA DE NIVEL ");
      Serial.print("desejado/atual: " + String(nivelDesejado) + "/" + String(nivelAtual) + " ");
      Serial.print((digitalRead(pinoBombaEncher)) ? "encher ON" : "encher OFF");
      Serial.print("/");
      Serial.println((digitalRead(pinoBombaEsvaziar)) ? "esvaziar ON" : "esvaziar OFF");
    }
    nodeSerial.println(nivelAtual);
    tempoAnterior = tempoAtual;
  }
}

// Bombas (função unificada)
void controlarBombas(bool encherLigada, bool esvaziarLigada) {
  digitalWrite(pinoBombaEncher, encherLigada ? HIGH : LOW);
  digitalWrite(pinoBombaEsvaziar, esvaziarLigada ? HIGH : LOW);
}

// Lógica
void tratarControleAutomatico() {
  atualizarNivel();

  if (!nivelAtingido && !pausado) {
    if (nivelAtual >= (nivelDesejado - toleranciaNivel) && nivelAtual <= (nivelDesejado + toleranciaNivel)) {
      controlarBombas(false, false);
      nivelAtingido = true;
      atualizarTela();
    } else if (nivelAtual > nivelDesejado) {
      controlarBombas(true, false); // encher
      atualizarTela();
    } else {
      controlarBombas(false, true); // esvaziar
      atualizarTela();
    }
  }
}

void tratarBotoesManuais() {
  bool encher = digitalRead(pinoBotaoEncher) == HIGH;
  bool esvaziar = digitalRead(pinoBotaoEsvaziar) == HIGH;

  if (encher && esvaziar) {
    controlarBombas(false, false);
    if (DEBUG) Serial.println("Aviso: ambos os botoes pressionados, bombas desligadas.");
    return;
  }

  if (encher || esvaziar) {
    controlarBombas(encher, esvaziar);
    nivelAtingido = true; // override manual
    atualizarNivel();
    atualizarTela();
  } else {
    controlarBombas(false, false);
  }
}

void tratarBotaoPause() {
  if (digitalRead(pinoPause) == HIGH) {
    controlarBombas(false, false);
    pausado = true;
    if (DEBUG) Serial.println("PAUSADO");
  } else {
    pausado = false;
  }
}

void tratarEmergencia() {
  if (digitalRead(pinoEmergencia) == HIGH) {
    if (DEBUG) Serial.println("Emergencia! Nivel muito alto!");
    controlarBombas(false, true); // força esvaziar
    delay(3000);
    controlarBombas(false, false);
    nivelAtingido = true;
    if (DEBUG) Serial.println("Encerrado.");
  }
}

// Comunicação
void tratarComunicacaoNode() {
  if (nodeSerial.available() > 0) {
    float nivelRecebido = nodeSerial.readStringUntil('\n').toFloat();
    definirNivelDesejado(nivelRecebido);
    if (DEBUG) {
      Serial.print("Recebido do NodeMCU: ");
      Serial.println(nivelRecebido);
    }
    atualizarTela(); // atualizar LCD quando novo valor chega
  }
}

void tratarEntradaSerialDebug() {
  if (Serial.available() > 0) {
    String entrada = Serial.readStringUntil('\n');
    if (entrada.startsWith("@")) {
      float nivelMonitor = entrada.substring(1).toFloat();
      definirNivelDesejado(nivelMonitor);
      if (DEBUG) {
        Serial.print("Nivel Desejado atualizado (debug): ");
        Serial.println(nivelMonitor);
      }
      atualizarTela(); // atualizar LCD quando novo valor chega
    }
  }
}

void definirNivelDesejado(float nivel) {
  if (nivel <= alturaTanque) {
    nivelDesejado = nivel;
    nivelAtingido = false; // automático volta
  } else {
    if (DEBUG) Serial.println("Nivel fora do escopo (0-" + String(alturaTanque) + ")");
  }
}
