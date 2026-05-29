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
    void salvar(String uid, String nome);
    void remover(String uid);
    bool autenticar(String uid, String &nomeOut);
    void limpar();
    
    // Retorna todos os usuários em formato JSON para o Dashboard
    String obterJsonLista();
};

extern UsuariosAPI Usuarios;

#endif