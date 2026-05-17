import mqtt from 'mqtt';
import { useTankStore } from '../store/useTankStore';

// Padrão Singleton: Mantém a instância única viva fora do ciclo do React
let client = null;

export const initMqtt = () => {
  // Se já estiver conectado ou conectando, aborta para não duplicar
  if (client) return; 

  const store = useTankStore.getState();
  store.setConnectionStatus('reconnecting');

  // Construção da URL WSS para HiveMQ Cloud
  const host = process.env.NEXT_PUBLIC_MQTT_SERVER;
  const port = 8884; // Porta obrigatória para WebSockets no HiveMQ
  const url = `wss://${host}:${port}/mqtt`;

  // Gera um ID único para cada aba do navegador aberta
  const clientId = `nexus_web_${Math.random().toString(16).substring(2, 8)}`;

  client = mqtt.connect(url, {
    username: process.env.NEXT_PUBLIC_MQTT_USER,
    password: process.env.NEXT_PUBLIC_MQTT_PASSWORD,
    clientId: clientId,
    reconnectPeriod: 3000, // Tenta reconectar a cada 3s se cair
    clean: true,
  });

  // ==========================================
  // EVENTOS DE CONEXÃO
  // ==========================================
  
  client.on('connect', () => {
    console.log('[MQTT] Conectado com sucesso via WebSockets!');
    useTankStore.getState().setConnectionStatus('connected');
    
    // Assina os tópicos da placa
    client.subscribe('tanque/telemetria');
    client.subscribe('tanque/logs');
  });

  client.on('reconnect', () => {
    useTankStore.getState().setConnectionStatus('reconnecting');
  });

  client.on('offline', () => {
    useTankStore.getState().setConnectionStatus('offline');
  });

  // ==========================================
  // EVENTOS DE RECEBIMENTO (LISTENER)
  // ==========================================

  client.on('message', (topic, message) => {
    try {
      const data = JSON.parse(message.toString());
      const currentStore = useTankStore.getState();

      if (topic === 'tanque/telemetria') {
        // Atualiza nível e status. A store já cuida de destravar a UI internamente.
        currentStore.setTelemetry(data.nivel, data.estado);
      } 
      else if (topic === 'tanque/logs') {
        currentStore.addLog(data);
      }
    } catch (error) {
      console.error('[MQTT] Falha ao processar pacote JSON:', error);
    }
  });
};

// ==========================================
// FUNÇÃO DE ENVIO (COMMAND EMITTER)
// ==========================================

/**
 * Envia um comando para a placa e gerencia o timeout da Interface.
 * @param {string} acao - ENCHER, ESVAZIAR, PARAR, SYNC_USER, etc.
 * @param {number} valor - Volume alvo (0-100). Padrão -1 se não aplicável.
 * @param {string} uid - UID do cartão RFID (para SYNC).
 * @param {string} nome - Nome do usuário (para SYNC).
 */
export const sendCommand = (acao, valor = -1, uid = "", nome = "") => {
  const store = useTankStore.getState();

  // Proteção: Não tenta enviar se não houver internet
  if (!client || !client.connected) {
    store.setCommandSending(false, "❌ Erro: Dashboard desconectado da nuvem.");
    return;
  }

  // 1. Trava a Interface informando que o comando está em trânsito
  store.setCommandSending(true, `Enviando comando [${acao}]...`);

  // 2. Monta o pacote e envia
  const payload = JSON.stringify({ comando: acao, valor, uid, nome });
  client.publish('tanque/comando', payload);

  // 3. O Relógio da Verdade (Timeout de 4 segundos)
  setTimeout(() => {
    // Pegamos o estado FRESCO da store 4 segundos depois
    const estadoAposEspera = useTankStore.getState();
    
    // Se isSending ainda for true, a placa não enviou telemetria de volta para nos destravar
    if (estadoAposEspera.isSending) {
      estadoAposEspera.setCommandSending(false, "⚠️ Erro: A placa não confirmou o comando a tempo (Timeout).");
    }
  }, 4000);
};