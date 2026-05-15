#ifndef USUARIOS_H
#define USUARIOS_H
#include <Arduino.h>
#include <Preferences.h>

class UsuariosAPI {
private:
    Preferences prefs;
public:
    void iniciar();
    void salvar(String uid, String nome);
    bool autenticar(String uid, String &nomeOut);
    void limpar();
};

extern UsuariosAPI Usuarios;
#endif