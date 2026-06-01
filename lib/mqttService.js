import mqtt from 'mqtt';
import { useTankStore } from '../store/useTankStore';

let client = null;

export const initMqtt = () => {
  if (client) return client;

  const store = useTankStore.getState();
  
  store.setMqttOk(false);
  store.setBoardOnline(false);

  const host = process.env.NEXT_PUBLIC_MQTT_URL;
  const user = process.env.NEXT_PUBLIC_MQTT_USER;
  const pass = process.env.NEXT_PUBLIC_MQTT_PASS;
  
  // Puxa o prefixo da variável global. Se não achar, usa um padrão de segurança.
  const topicPrefix = process.env.NEXT_PUBLIC_MQTT_TOPIC_PREFIX || 'nx_tanque_8f7d6c5b';

  const url = (host && host.startsWith('ws')) 
    ? host 
    : `wss://${host || 'broker.hivemq.com'}:8884/mqtt`;

  const clientId = `nexus_web_${Date.now()}_${Math.floor(Math.random() * 1000)}`;

  client = mqtt.connect(url, {
    username: user,
    password: pass,
    clientId: clientId,
    reconnectPeriod: 4000,
    connectTimeout: 10000,
    clean: true,
  });

  if (typeof window !== 'undefined') {
    window.addEventListener('beforeunload', () => {
      if (client) client.end(true);
    });
  }

  client.on('connect', () => {
    console.log('[MQTT] Dashboard conectado com ID:', clientId);
    useTankStore.getState().setMqttOk(true);

    // Inscrições dinâmicas baseadas na variável global
    client.subscribe(`${topicPrefix}/telemetria`);
    client.subscribe(`${topicPrefix}/logs`);
    client.subscribe(`${topicPrefix}/status`);
    client.subscribe(`${topicPrefix}/telemetria/parametros`); 
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

      // Comparações dinâmicas
      if (topic === `${topicPrefix}/status`) {
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
      else if (topic === `${topicPrefix}/telemetria`) {
        currentStore.setTelemetry(data.nivel, data.estado);
      } 
      else if (topic === `${topicPrefix}/logs`) {
        currentStore.addLog(data);
      }
      else if (topic === `${topicPrefix}/telemetria/parametros`) { 
        currentStore.setDeviceParams(data);
      }
    } catch (e) {
      console.error('[MQTT] Erro ao processar mensagem JSON:', e);
    }
  });

  return client;
};

export const sendCommand = (acao, valor = -1, uid = "", nome = "", extraPayload = {}) => {
  if (!client || !client.connected) {
    console.warn("MQTT não conectado");
    return false;
  }

  const store = useTankStore.getState();
  store.setCommandSending(true);

  // Usa a variável na hora de enviar comandos
  const topicPrefix = process.env.NEXT_PUBLIC_MQTT_TOPIC_PREFIX || 'nx_tanque_8f7d6c5b';
  const payload = { comando: acao, valor, uid, nome, ...extraPayload };
  
  client.publish(`${topicPrefix}/comando`, JSON.stringify(payload));

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