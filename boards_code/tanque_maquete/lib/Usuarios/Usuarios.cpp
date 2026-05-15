#include "Usuarios.h"

UsuariosAPI Usuarios;

void UsuariosAPI::iniciar() {
    prefs.begin("usuarios", false); // Abre o espaço na memória Flash
}

void UsuariosAPI::salvar(String uid, String nome) {
    prefs.putString(uid.c_str(), nome); // Salva o par: UID -> Nome
}

bool UsuariosAPI::autenticar(String uid, String &nomeOut) {
    if (prefs.isKey(uid.c_str())) {
        nomeOut = prefs.getString(uid.c_str());
        return true;
    }
    return false;
}

void UsuariosAPI::limpar() {
    prefs.clear();
}