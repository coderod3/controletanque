#include "Usuarios.h"

UsuariosAPI Usuarios;

void UsuariosAPI::iniciar() {
    prefs.begin("usuarios", false); 
}

String UsuariosAPI::getListaUIDs() {
    return prefs.getString("idx_uids", "");
}

void UsuariosAPI::setListaUIDs(String lista) {
    prefs.putString("idx_uids", lista);
}

void UsuariosAPI::salvar(String uid, String nome) {
    prefs.putString(uid.c_str(), nome);
    
    String lista = getListaUIDs();
    // Se o UID não está na lista, adiciona separado por vírgula
    if (lista.indexOf(uid + ",") == -1) {
        lista += uid + ",";
        setListaUIDs(lista);
    }
}

void UsuariosAPI::remover(String uid) {
    prefs.remove(uid.c_str()); // Remove nome
    
    String lista = getListaUIDs();
    lista.replace(uid + ",", ""); // Tira do index
    setListaUIDs(lista);
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

String UsuariosAPI::obterJsonLista() {
    String lista = getListaUIDs();
    String json = "[";
    
    int start = 0;
    int index = lista.indexOf(',');
    bool primeiro = true;

    while (index != -1) {
        String uid = lista.substring(start, index);
        String nome = prefs.getString(uid.c_str(), "Desconhecido");
        
        if (!primeiro) json += ",";
        json += "{\"uid\":\"" + uid + "\",\"nome\":\"" + nome + "\"}";
        
        primeiro = false;
        start = index + 1;
        index = lista.indexOf(',', start);
    }
    
    json += "]";
    return json;
}