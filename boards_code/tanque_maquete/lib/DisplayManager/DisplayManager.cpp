#include "HardwareMap.h"
#include "DisplayManager.h"

DisplayManager::DisplayManager() : _lcd(0x27, 16, 2) {}

void DisplayManager::init() {
    _lcd.init();
    _lcd.backlight();
    _lcd.clear();
    showStatus("NEXUS SYSTEM", "INICIALIZANDO...");
}

void DisplayManager::showStatus(String line1, String line2) {
    // Se o que eu quero escrever é igual ao que já está lá, não faço nada!
    if (line1 == _lastLine1 && line2 == _lastLine2) {
        return; 
    }

    // Se chegou aqui, algo mudou. Atualizamos o cache e a tela.
    _lastLine1 = line1;
    _lastLine2 = line2;

    // Removemos o clear() para eliminar o flicker de vez
    _lcd.setCursor(0, 0);
    String p1 = line1 + "                "; 
    _lcd.print(p1.substring(0, 16));
    
    _lcd.setCursor(0, 1);
    String p2 = line2 + "                ";
    _lcd.print(p2.substring(0, 16));
}

void DisplayManager::showIdle(float volume) {
    _lcd.setCursor(0, 0);
    _lcd.print("SISTEMA PRONTO  "); 
    _lcd.setCursor(0, 1);
    _lcd.print("NIVEL: ");
    _lcd.print(volume, 1); 
    _lcd.print(" L     ");
}

// === NOVO FLUXO DE CONFIGURAÇÃO (WIZARD) ===

// Layout solicitado: Linha 1 Encher, Linha 2 Esvaziar
void DisplayManager::showConfigDir(bool enchendo) {
    _lcd.setCursor(0, 0);
    _lcd.print(enchendo ? "> 1. ENCHER    " : "  1. encher    ");
    _lcd.setCursor(0, 1);
    _lcd.print(enchendo ? "  2. esvaziar  " : "> 2. ESVAZIAR  ");
}

void DisplayManager::showConfigVol(int litros) {
    _lcd.setCursor(0, 0);
    _lcd.print("QUANTIDADE:     ");
    _lcd.setCursor(0, 1);
    _lcd.print("<    [ ");
    _lcd.print(litros);
    _lcd.print(" L ]    >");
}

void DisplayManager::showConfigConfirm(int litros, bool enchendo) {
    _lcd.setCursor(0, 0);
    _lcd.print("CONFIRMAR? [OK] ");
    _lcd.setCursor(0, 1);
    _lcd.print(litros);
    _lcd.print(enchendo ? "L ENCHER       " : "L ESVAZIAR     ");
}

// Mantive as outras funções (showIdle, showExecuting, etc) iguais, 
// apenas garanta que elas usem espaços no final para limpar a linha.

// ===========================================

void DisplayManager::showExecuting(float atual, float alvo, bool enchendo) {
    _lcd.setCursor(0, 0);
    _lcd.print(enchendo ? "ENCHENDO...     " : "ESVAZIANDO...   ");
    _lcd.setCursor(0, 1);
    _lcd.print(atual, 1);
    _lcd.print("L -> ");
    _lcd.print(alvo, 1);
    _lcd.print("L   ");
}

void DisplayManager::showErrorMessage(String msg) {
    showStatus("ERRO DE SISTEMA", msg);
}

void DisplayManager::showEmergency() {
    showStatus("!!! PERIGO !!!", "TRANSBORDO DETEC");
}

DisplayManager display;