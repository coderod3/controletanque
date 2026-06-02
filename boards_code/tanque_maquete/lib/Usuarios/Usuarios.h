#ifndef USUARIOS_H
#define USUARIOS_H

#include <Arduino.h>
#include <Preferences.h>

class UsuariosAPI {
private:
    Preferences prefs;
    String getListaUIDs();
    void setListaUIDs(String lista);

public:
    void iniciar();
    // Agora recebe e grava os limites físicos
    void salvar(String uid, String nome, float lim_encher, float lim_esvaziar);
    void remover(String uid);
    // Retorna os limites por referência para a main usar
    bool autenticar(String uid, String &nomeOut, float &lim_encher, float &lim_esvaziar);
    void limpar();
    
    String obterJsonLista();
};

extern UsuariosAPI Usuarios;

#endif