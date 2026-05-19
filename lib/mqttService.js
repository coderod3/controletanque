import mqtt from 'mqtt';
import { useTankStore } from '../store/useTankStore';

let client = null;

export const initMqtt = () => {
  if (client) return; 

  const store = useTankStore.getState();
  store.setConnectionStatus('reconnecting');

  const host = process.env.NEXT_PUBLIC_MQTT_SERVER;
  const port = 8884;
  const url = `wss://${host}:${port}/mqtt`;

  const clientId = `nexus_web_${Math.random().toString(16).substring(2, 8)}`;

  client = mqtt.connect(url, {
    username: process.env.NEXT_PUBLIC_MQTT_USER,
    password: process.env.NEXT_PUBLIC_MQTT_PASSWORD,
    clientId: clientId,
    reconnectPeriod: 3000,
    clean: true,
  });

  // ==========================================
  // EVENTOS DE CONEXÃO
  // ==========================================
  
  client.on('connect', () => {
    console.log('[MQTT] Conectado com sucesso via WebSockets!');
    useTankStore.getState().setConnectionStatus('connected');
    
    client.subscribe('tanque/telemetria');
    client.subscribe('tanque/logs');
  });

  client.on('reconnect', () => {
    console.log('[MQTT] Tentando reconectar...');
    useTankStore.getState().setConnectionStatus('reconnecting');
  });

  client.on('offline', () => {
    console.warn('[MQTT] Cliente offline');
    useTankStore.getState().setConnectionStatus('offline');
  });

  client.on('close', () => {
    console.warn('[MQTT] Conexão fechada');
    useTankStore.getState().setConnectionStatus('offline');
  });

  client.on('error', (error) => {
    console.error('[MQTT] Erro de conexão:', error);
    useTankStore.getState().setConnectionStatus('offline');
  });

  // ==========================================
  // EVENTOS DE RECEBIMENTO
  // ==========================================

  client.on('message', (topic, message) => {
    try {
      const data = JSON.parse(message.toString());
      const currentStore = useTankStore.getState();

      if (topic === 'tanque/telemetria') {
        currentStore.setTelemetry(data.nivel, data.estado);
        // Atualizar indicador de conexão quando telemetria chega
        if (currentStore.connectionStatus !== 'connected') {
          currentStore.setConnectionStatus('connected');
        }
      } 
      else if (topic === 'tanque/logs') {
        currentStore.addLog(data);
      }
    } catch (error) {
      console.error('[MQTT] Falha ao processar JSON:', error);
    }
  });
};

// ==========================================
// FUNÇÃO DE ENVIO
// ==========================================

export const sendCommand = (acao, valor = -1, uid = "", nome = "") => {
  const store = useTankStore.getState();

  if (!client || !client.connected) {
    store.setCommandSending(false, "❌ Erro: Dashboard desconectado da nuvem.");
    return;
  }

  store.setCommandSending(true, `Enviando comando [${acao}]...`);

  const payload = JSON.stringify({ comando: acao, valor, uid, nome });
  client.publish('tanque/comando', payload);

  setTimeout(() => {
    const estadoAposEspera = useTankStore.getState();
    if (estadoAposEspera.isSending) {
      estadoAposEspera.setCommandSending(false, "⚠️ Erro: A placa não confirmou o comando (Timeout).");
    }
  }, 4000);
};

export const disconnectMqtt = () => {
  if (client) {
    client.end();
    client = null;
  }
};