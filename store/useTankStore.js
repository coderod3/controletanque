import { create } from 'zustand';

export const useTankStore = create((set) => ({
  // ==========================================
  // 1. ESTADO GLOBAL (VARIÁVEIS)
  // ==========================================
  
  // Status da Conexão MQTT
  connectionStatus: 'offline', // 'connected' | 'reconnecting' | 'offline'
  
  // Dados Vitais do Tanque (Digital Twin)
  volume: null, 
  operationStatus: 'desconhecido', 
  lastUpdate: null,

  // Controle de Bloqueio de UI (Anti-Spam / UX)
  isSending: false,
  commandFeedback: '', 

  // Histórico de Logs em Tempo Real
  logs: [],

  // ==========================================
  // 2. AÇÕES (MÉTODOS DE ATUALIZAÇÃO)
  // ==========================================

  setConnectionStatus: (status) => set({ connectionStatus: status }),
  
  setTelemetry: (vol, status) => set((state) => ({
    volume: vol,
    operationStatus: status,
    lastUpdate: Date.now(),
    // MÁGICA DE UX: Se chegou telemetria, destrava os botões!
    isSending: false, 
    commandFeedback: state.isSending ? 'Ação confirmada pela placa!' : state.commandFeedback
  })),

  setCommandSending: (isSending, msg) => set({ 
    isSending, 
    commandFeedback: msg 
  }),

  addLog: (logObj) => set((state) => {
    // Garante formato consistente: { time: timestamp, msg: string, tipo: string }
    const newLog = {
      time: logObj.time || Date.now(),
      msg: logObj.msg || 'Log sem mensagem',
      tipo: logObj.tipo || 'info'
    };

    // Adiciona no topo (unshift lógico) e mantém apenas os últimos 50
    const novosLogs = [newLog, ...state.logs].slice(0, 50);
    return { logs: novosLogs };
  })
}));