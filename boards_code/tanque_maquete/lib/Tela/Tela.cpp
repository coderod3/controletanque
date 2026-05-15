#include "Tela.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// O endereço padrão costuma ser 0x27 ou 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2); 
TelaAPI Tela;

void TelaAPI::iniciar() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    ultimaAtualizacao = 0;
}

void TelaAPI::limpar() {
    lcd.clear();
    ultimaLinha1 = "";
    ultimaLinha2 = "";
}

void TelaAPI::atualizar(String linha1, String linha2) {
    // Só gasta tempo processando se o texto mudou
    if (linha1 == ultimaLinha1 && linha2 == ultimaLinha2) return;
    
    // Proteção contra flicker (atraso mínimo entre escritas)
    if (millis() - ultimaAtualizacao < 150) return;
    ultimaAtualizacao = millis();

    if (linha1 != ultimaLinha1) {
        lcd.setCursor(0, 0);
        String p1 = linha1;
        while(p1.length() < 16) p1 += " "; // Preenche com espaços para limpar rastro
        lcd.print(p1);
        ultimaLinha1 = linha1;
    }

    if (linha2 != ultimaLinha2) {
        lcd.setCursor(0, 1);
        String p2 = linha2;
        while(p2.length() < 16) p2 += " ";
        lcd.print(p2);
        ultimaLinha2 = linha2;
    }
}