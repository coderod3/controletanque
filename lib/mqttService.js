import mqtt from 'mqtt';
import { useTankStore } from '../store/useTankStore';

let client = null;

export const initMqtt = () => {
  if (client) return client;

  const store = useTankStore.getState();
  
  store.setMqttOk(false);
  store.setBoardOnline(false);

  const url = `wss://${process.env.NEXT_PUBLIC_MQTT_SERVER}:8884/mqtt`;

  // FIX: ID único e persistente na sessão do navegador para evitar conflito com a placa
  const clientId = `nexus_web_${Date.now()}_${Math.floor(Math.random() * 1000)}`;

  client = mqtt.connect(url, {
    username: process.env.NEXT_PUBLIC_MQTT_USER,
    password: process.env.NEXT_PUBLIC_MQTT_PASSWORD,
    clientId: clientId,
    reconnectPeriod: 4000,
    connectTimeout: 10000,
    clean: true,
  });

  // FIX: Garante que, ao atualizar a página (F5), o broker seja avisado para liberar a vaga imediatamente
  if (typeof window !== 'undefined') {
    window.addEventListener('beforeunload', () => {
      if (client) client.end(true);
    });
  }

  client.on('connect', () => {
    console.log('[MQTT] Dashboard conectado com ID:', clientId);
    useTankStore.getState().setMqttOk(true);

    client.subscribe('tanque/telemetria');
    client.subscribe('tanque/logs');
    client.subscribe('tanque/status');
    client.subscribe('tanque/telemetria/parametros'); // NOVO OUVINTE
  });

  client.on('reconnect', () => {
    useTankStore.getState().setMqttOk(false);
  });

  client.on('offline', () => {
    useTankStore.getState().setMqttOk(false);
    useTankStore.getState().setBoardOnline(false);
  });

  client.on('message', (topic, message) => {
    try {
      const data = JSON.parse(message.toString());
      const currentStore = useTankStore.getState();

      if (topic === 'tanque/status') {
        if (data.status === 'ONLINE') {
          currentStore.setBoardOnline(true);
          currentStore.addLog({ tipo: 'success', fonte: 'SISTEMA', msg: 'ESP32 reconectado à rede.' });
        } 
        else if (data.status === 'OFFLINE') {
          currentStore.setBoardOnline(false);
          currentStore.setTelemetry(currentStore.volume, 'idle');
          currentStore.addLog({ tipo: 'error', fonte: 'SISTEMA', msg: 'Falha crítica: ESP32 offline (Sinal LWT).' });
        }
      } 
      else if (topic === 'tanque/telemetria') {
        currentStore.setTelemetry(data.nivel, data.estado);
      } 
      else if (topic === 'tanque/logs') {
        currentStore.addLog(data);
      }
      else if (topic === 'tanque/telemetria/parametros') { // SALVA O QUE VEM DA NVS
        currentStore.setDeviceParams(data);
      }
    } catch (e) {
      console.error('[MQTT] Erro ao processar mensagem JSON:', e);
    }
  });

  return client;
};

// Permite enviar um payload customizado (para salvar os dicionários da página de config)
export const sendCommand = (acao, valor = -1, uid = "", nome = "", extraPayload = {}) => {
  if (!client || !client.connected) {
    console.warn("MQTT não conectado");
    return false;
  }

  const store = useTankStore.getState();
  store.setCommandSending(true);

  // Espalha as configurações extra no JSON que vai para a placa
  const payload = { comando: acao, valor, uid, nome, ...extraPayload };
  client.publish('tanque/comando', JSON.stringify(payload));

  // Timeout de segurança
  setTimeout(() => {
    const estadoAtual = useTankStore.getState();
    
    if (estadoAtual.isSending) {
      store.setCommandSending(false);
      store.setTelemetry(estadoAtual.volume, 'idle');
      store.addLog({
        tipo: 'error', 
        fonte: 'SISTEMA', 
        msg: 'Timeout: A placa não confirmou a operação. Cancelada na interface.' 
      });
    }
  }, 5000);

  return true;
};