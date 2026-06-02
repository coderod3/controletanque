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

void UsuariosAPI::salvar(String uid, String nome, float lim_encher, float lim_esvaziar) {
    // Compacta os dados: "João Silva|30.5|15.0"
    String payload = nome + "|" + String(lim_encher, 1) + "|" + String(lim_esvaziar, 1);
    prefs.putString(uid.c_str(), payload);
    
    String lista = getListaUIDs();
    if (lista.indexOf(uid + ",") == -1) {
        lista += uid + ",";
        setListaUIDs(lista);
    }
}

void UsuariosAPI::remover(String uid) {
    prefs.remove(uid.c_str()); 
    String lista = getListaUIDs();
    lista.replace(uid + ",", ""); 
    setListaUIDs(lista);
}

bool UsuariosAPI::autenticar(String uid, String &nomeOut, float &lim_encher, float &lim_esvaziar) {
    if (prefs.isKey(uid.c_str())) {
        String payload = prefs.getString(uid.c_str());
        
        int p1 = payload.indexOf('|');
        int p2 = payload.indexOf('|', p1 + 1);
        
        if (p1 != -1 && p2 != -1) {
            nomeOut = payload.substring(0, p1);
            lim_encher = payload.substring(p1 + 1, p2).toFloat();
            lim_esvaziar = payload.substring(p2 + 1).toFloat();
        } else {
            // Fallback caso seja um usuário salvo na versão antiga do código
            nomeOut = payload; 
            lim_encher = 100.0;
            lim_esvaziar = 100.0;
        }
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
        String payload = prefs.getString(uid.c_str(), "Desconhecido|0|0");
        
        String nome = payload;
        float le = 0.0, ld = 0.0;
        
        int p1 = payload.indexOf('|');
        int p2 = payload.indexOf('|', p1 + 1);
        if (p1 != -1 && p2 != -1) {
            nome = payload.substring(0, p1);
            le = payload.substring(p1 + 1, p2).toFloat();
            ld = payload.substring(p2 + 1).toFloat();
        }
        
        if (!primeiro) json += ",";
        json += "{\"uid\":\"" + uid + "\",\"nome\":\"" + nome + "\",\"limite_encher\":" + String(le, 1) + ",\"limite_esvaziar\":" + String(ld, 1) + "}";
        
        primeiro = false;
        start = index + 1;
        index = lista.indexOf(',', start);
    }
    
    json += "]";
    return json;
}