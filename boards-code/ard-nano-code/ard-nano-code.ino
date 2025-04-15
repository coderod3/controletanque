// Inclui bibliotecas
#include <Wire.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

// Inicializa o LCD com seu endereço I2C (0x27) e dimensões (16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);
// Inicializa a segunda serial para comunicação com NodeMCU
SoftwareSerial nodeSerial(2, 3);  // RX, TX

// Definição dos pinos
const int pinoTrigger = 6;         // GPIO4 do sensor ultrassônico
const int pinoEcho = 7;            // GPIO0 do sensor ultrassônico
const int pinoBombaEncher = 9;     // Bomba para encher o tanque
const int pinoBombaEsvaziar = 10;  // Bomba para esvaziar o tanque
// bom pra por resistor
const int pinoBotaoEncher = 4;    // Botão para encher
const int pinoBotaoEsvaziar = 5;  // Botão para esvaziar
const int pinoEmergencia = 8;     // Pino de emergência
const int terceiroBotao = 12;

const float alturaTanque = 20;  // Altura do tanque em cm

// Variáveis de nível de água
bool nivelAtingido = false;  // Indica se o nível atual da água está dentro do valor esperado
bool pausado = false;
float nivelAtual = 0;       // Nível atual da água (em cm) medido pelo sensor
float nivelDesejado = 5;    // Nível desejado da água (em cm)
float nivelRecebido = 0;    // Nível recebido pelo NodeMCU
float toleranciaNivel = 1;  // Tolerância aceitável para o nível de água

// Variáveis dos botões
int estadoUltimoBotaoEncher = 0;
int estadoUltimoBotaoEsvaziar = 0;
int estadoUltimoTerceiroBotao = 0;
int estadoBotaoEncher = 0;
int estadoBotaoEsvaziar = 0;
int estadoTerceiroBotao = 0;

// Variável de emergência
int estadoPinoEmergencia = 0;

// Variáveis de controle de tempo
unsigned long tempoAnterior = 0;
const long intervaloEnvio = 1000;  // Intervalo para enviar dados (1 segundo)

void setup() {
  Serial.begin(9600);
  nodeSerial.begin(9600);  // Comunicação serial com NodeMCU

  // Configuração dos pinos
  pinMode(pinoTrigger, OUTPUT);
  pinMode(pinoEcho, INPUT);
  pinMode(pinoBombaEncher, OUTPUT);
  pinMode(pinoBombaEsvaziar, OUTPUT);
  pinMode(pinoBotaoEncher, INPUT);
  pinMode(pinoBotaoEsvaziar, INPUT);
  pinMode(pinoEmergencia, INPUT);
  pinMode(terceiroBotao, INPUT);

  // Inicialização de variáveis locais
  delay(50);
  nivelAtual = medirDistancia();
  nivelDesejado = nivelAtual;

  // Inicialização do LCD
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print("Tanque  100L");
  lcd.setCursor(1, 1);
  lcd.print("Nivel: " + String((nivelAtual / alturaTanque) * 100));
  lcd.setCursor(14, 1);
  lcd.print("L");
}

void loop() {

  // Muda o nível desejado pelo monitor serial com comando '@'
  // função para mandar coisa pro node que pode chamar dentro das bombas
  if (Serial.available() > 0) {
    //String dadosEntrada = Serial.readStringUntil('\n');  // Lê dados até a nova linha
    String nivelMonitor = Serial.readStringUntil('\n');
    //if (dadosEntrada.startsWith("@")) {
    //String nivelMonitor = dadosEntrada.substring(1);
    nodeSerial.println(nivelMonitor);
    definirNivelDesejado(nivelMonitor.toFloat());  // Remove o prefixo '@'
    Serial.print("para o node/desejado: ");
    Serial.println(nivelMonitor);
    /*} else if (dadosEntrada.startsWith("@")){
      float nivelMonitor = dadosEntrada.substring(1).toFloat();
      distancia = nivelMonitor;
      Serial.println(nivelMonitor);
    }*/
  }

  // Checa se há novos dados disponíveis no SoftwareSerial do node que vem do site se alguem setar nivel
  if (nodeSerial.available() > 0) {

    delay(50);
    String nr = nodeSerial.readStringUntil('\n');  // Lê dados até a nova linha
    float nivelRecebido = nr.toFloat();
    // Adicionar aqui bloqueio de requests se as bombas ja estiverem ocupadas
    definirNivelDesejado(nivelRecebido);  //define novo nivel pra chegar
    Serial.print("Recebido: ");
    Serial.println(nivelRecebido);

  }

  // Leitura dos botões
  estadoBotaoEncher = digitalRead(pinoBotaoEncher);
  estadoBotaoEsvaziar = digitalRead(pinoBotaoEsvaziar);
  estadoPinoEmergencia = digitalRead(pinoEmergencia);
  estadoTerceiroBotao = digitalRead(terceiroBotao);

  // Verifica estado de emergência
  if (estadoPinoEmergencia == HIGH) {
    pararEmergencia();
  }

  if (estadoTerceiroBotao == HIGH) {
    desligarBombas(0);
    Serial.println("PAUSADO");
  } else {
    // Controle do botão de encher
    if (estadoUltimoBotaoEncher != estadoBotaoEncher) {

      delay(50);  // Tempo para debouncing
      if (estadoBotaoEncher == HIGH) {
        nivelAtingido = true;  // Desativa outros controles
        ligarBombaEncher();
      } else {
        desligarBombas(1);
      }
      estadoUltimoBotaoEncher = estadoBotaoEncher;

    }

    // Controle do botão de esvaziar
    if (estadoUltimoBotaoEsvaziar != estadoBotaoEsvaziar) {

      delay(50);  // Tempo para debouncing
      if (estadoBotaoEsvaziar == HIGH) {
        nivelAtingido = true;  // Desativa outros controles
        ligarBombaEsvaziar();
      } else {
        desligarBombas(1);
      }
      estadoUltimoBotaoEsvaziar = estadoBotaoEsvaziar;

    }

    // Lógica de controle de nível
    if (nivelAtingido == false) {
      if (nivelAtual >= (nivelDesejado - toleranciaNivel) && nivelAtual <= (nivelDesejado + toleranciaNivel)) {
        desligarBombas(1);
        nivelAtual = nivelDesejado;
      }
      else if (nivelAtual > nivelDesejado) {
        ligarBombaEncher();
      }
      else {
        ligarBombaEsvaziar();
      }
      
      delay(50);  // Pequeno atraso para estabilizar as bombas
    }

    // DELAY DE 1 SEGUNDO ASYNC PRINTS
    unsigned long tempoAtual = millis();
    if (tempoAtual - tempoAnterior >= intervaloEnvio) {

      // Impressões de estado
      Serial.print((nivelAtingido) ? "Aceitável " : "FORA DE NÍVEL ");
      Serial.print("desejado/atual: " + String(nivelDesejado) + "/" + String(nivelAtual) + " ");
      // Estado das bombas
      Serial.print((digitalRead(pinoBombaEncher)) ? "ligada" : "desligada");
      Serial.print("/");
      Serial.println((digitalRead(pinoBombaEsvaziar)) ? "ligada" : "desligada");
      Serial.println();

      // Chama a função de atualização da tela se qualquer bomba estiver ligada
      if (digitalRead(pinoBombaEncher) == HIGH || digitalRead(pinoBombaEsvaziar) == HIGH) {
        atualizarTela();
        enviarNode();
        // Serial.println("AT Tela");
      }

      tempoAnterior = tempoAtual;
    }
  }
}
  // Funções

  void enviarNode() {
    String nivelsensor = String(medirDistancia());
    delay(50);
    nodeSerial.println(nivelsensor);
    delay(10);
  }

  void atualizarTela() {
    nivelAtual = medirDistancia();
    lcd.setCursor(8, 1);
    lcd.print("      ");
    lcd.setCursor(8, 1);
    lcd.print(String(((nivelAtual / alturaTanque) * 100), 2));  // Mostra o nível em %
    delay(100);
  }

  void pararEmergencia() {
    Serial.println("Emergencia! Nível muito alto!");
    nivelAtingido = true;
    desligarBombas(1);
    digitalWrite(pinoTrigger, LOW);
    delay(100);  // Desliga pinos do sensor
    digitalWrite(pinoEcho, LOW);
    digitalWrite(pinoBombaEsvaziar, HIGH);
    delay(3000);  // Liga esvaziar por 2 segundos
    digitalWrite(pinoBombaEsvaziar, LOW);
    Serial.println("Encerrado.");
  }

  void ligarBombaEncher() {
    digitalWrite(pinoBombaEsvaziar, LOW);
    delay(100);
    digitalWrite(pinoBombaEsvaziar, LOW);
    delay(30);
    digitalWrite(pinoBombaEncher, HIGH);
    delay(100);
    digitalWrite(pinoBombaEncher, HIGH);
    delay(10);
  }

  void ligarBombaEsvaziar() {
    digitalWrite(pinoBombaEncher, LOW);
    delay(100);
    digitalWrite(pinoBombaEncher, LOW);
    delay(30);
    digitalWrite(pinoBombaEsvaziar, HIGH);
    delay(100);
    digitalWrite(pinoBombaEsvaziar, HIGH);
    delay(10);
  }

  void desligarBombas(int num) {
    if (num) {
      nivelAtingido = true;
    }
    digitalWrite(pinoBombaEncher, LOW);
    delay(100);
    digitalWrite(pinoBombaEncher, LOW);
    delay(50);
    digitalWrite(pinoBombaEsvaziar, LOW);
    delay(100);
    digitalWrite(pinoBombaEsvaziar, LOW);
  }

  void definirNivelDesejado(float nivel) {
    if (nivel <= alturaTanque) {
      nivelDesejado = nivel;
      nivelAtingido = false;
      Serial.print("Nivel Desejado(dentro funcao): ");
      Serial.println(nivelDesejado);
    } else {
      Serial.println("Número fora do escopo do tanque de 0 a " + String(alturaTanque));
    }
  }

  float medirDistancia() {
    // Envia pulso ultrassônico
    digitalWrite(pinoTrigger, LOW);
    delayMicroseconds(2);
    digitalWrite(pinoTrigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinoTrigger, LOW);

    // Mede a duração do eco
    long duracao = pulseIn(pinoEcho, HIGH);

    // Calcula a distância
    float distancia = duracao * 0.034 / 2;  // Velocidade do som no ar é 0.034 cm/µs
    // if(distancia <= tankheight){} // adicionar para por precaucao
    return distancia;
  }