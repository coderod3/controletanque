import { create } from 'zustand';

export const useTankStore = create((set, get) => ({
  // ==========================================
  // ESTADO GLOBAL
  // ==========================================
  capacidadeMaxima: 100.0,
  
  // NOVO: 'visualizacao' | 'operador' | 'gestor'
  userRole: 'gestor',

  mqttOk: false,           
  boardOnline: false,      

  volume: 0.0,
  targetVolume: 0.0,
  operationStatus: 'idle', 

  isSending: false,        
  lastUpdate: null,

  logs: [],

  // ==========================================
  // ACTIONS
  // ==========================================
  // NOVO SETTER DE CARGO
  setUserRole: (role) => set({ userRole: role }),

  setMqttOk: (status) => set({ mqttOk: status }),
  setBoardOnline: (status) => set({ boardOnline: status }),

  setTelemetry: (vol, status) => set((state) => ({
    volume: Number(vol) || 0,
    operationStatus: typeof status === 'string' ? status.toLowerCase() : 'unknown',
    lastUpdate: Date.now(),
    isSending: false,
  })),

  setTargetVolume: (target) => set({ targetVolume: Number(target) || 0 }),

  setCommandSending: (isSending) => set({ isSending }),

  setConnectionStatus: (status) => set({ 
    mqttOk: status === 'connected' || status === 'reconnecting'
  }),

  addLog: (logObj) => set((state) => {
    const timeString = new Date().toTimeString().slice(0, 8);
    
    const newLog = {
      id: Date.now(),
      time: timeString,
      msg: logObj.msg || logObj.message || 'Sem mensagem',
      tipo: logObj.tipo || 'info',
      fonte: logObj.fonte || 'SISTEMA'
    };

    return { 
      logs: [newLog, ...state.logs].slice(0, 50) 
    };
  }),

  reset: () => set({
    volume: 0,
    targetVolume: 0,
    operationStatus: 'idle',
    isSending: false,
    logs: []
  })
}));