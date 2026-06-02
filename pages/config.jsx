import { useState } from "react";
import { useTankStore } from "../store/useTankStore";
import { sendCommand } from "../lib/mqttService";
import Sidebar from "../components/Sidebar";
import Head from "next/head";

export default function Config() {
  const [sidebar, setSidebar] = useState(true);
  const boardOnline = useTankStore((state) => state.boardOnline);
  const userRole = useTankStore((state) => state.userRole);

  const [activeTab, setActiveTab] = useState("geometria");

  // Rascunho Local
  const [params, setParams] = useState({
    TANK_HEIGHT_EMPTY: 100.0,
    TANK_HEIGHT_FULL: 2.0,
    TANK_MAX_DIST: 110.0,
    TANK_MAX_VOLUME: 100.0,
    TANK_SAFE_MARGIN: 0.5,
    SENSOR_SAMPLES: 8,
    SENSOR_ECHO_DELAY_MS: 40,
    SENSOR_MAX_SLEW_RATE_CM_S: 6.0,
    SENSOR_READ_INTERVAL: 120,
    TELEMETRY_IDLE_DELTA_L: 1.5,
    TELEMETRY_IDLE_INTERVAL_MS: 10000,
    TELEMETRY_EXEC_DELTA_L: 0.8,
    TELEMETRY_EXEC_INTERVAL_MS: 400,
  });

  const [hasChanges, setHasChanges] = useState(false);

  // Mágica sem useEffect: Se não houver alteração pendente, exibe o que está na Placa (Store Global)
  const deviceParams = useTankStore((state) => state.deviceParams);
  const displayParams = hasChanges ? params : (deviceParams || params);

  const handleChange = (key, value) => {
    // Clona o estado de exibição atual para iniciar o rascunho de edições
    setParams({ ...displayParams, [key]: parseFloat(value) || value });
    setHasChanges(true);
  };

  // Funções de Disparo MQTT
  // Funções de Disparo MQTT
  const handleSync = () => {
    sendCommand("GET_SYNC");
    setHasChanges(false);
    useTankStore.getState().addLog({ tipo: 'info', fonte: 'SISTEMA', msg: 'Solicitando parâmetros da placa...' });
  };

  const handleSave = () => {
    // Converte os textos do form para Numéricos Puros
    const numericParams = {};
    for (let key in displayParams) {
        numericParams[key] = Number(displayParams[key]);
    }
    
    // ATENÇÃO AQUI: Nós enviamos numericParams "solto" como quinto argumento,
    // sem embrulhar em { payload: ... } para casar com a leitura do C++
    sendCommand("SET_PARAM", -1, "WEB", "Admin", numericParams);
    
    setHasChanges(false);
    useTankStore.getState().addLog({ tipo: 'success', fonte: 'WEB', msg: 'Parâmetros enviados e gravados na Flash com sucesso.' });
  };

  const handleFormatNVS = () => {
    // Usamos confirm nativo apenas para proteção extrema de sistema
    if (window.confirm("Aviso: Esta ação apagará permanentemente todos os cartões RFID da placa. Continuar?")) {
      sendCommand("LIMPAR_MEMORIA");
    }
  };

  const handleEmergencyStop = () => {
    sendCommand("PARAR");
  };

  // Componente de Linha (Lê os dados do DisplayParams)
  const ParamRow = ({ label, desc, paramKey, unit, step = "0.1", type = "number" }) => (
    <div className="flex flex-col sm:flex-row sm:items-center justify-between py-4 border-b border-slate-100 last:border-0 gap-4 hover:bg-slate-50/50 transition-colors px-3 -mx-3 rounded-xl">
      <div className="flex-1 pr-4">
        <label className="text-sm font-bold text-slate-800 tracking-tight">{label}</label>
        <p className="text-[11px] text-slate-500 mt-1 leading-snug">{desc}</p>
      </div>
      <div className="flex items-stretch shrink-0 shadow-sm rounded-lg overflow-hidden focus-within:ring-2 focus-within:ring-blue-500 focus-within:ring-offset-1 transition-shadow">
        <input 
          type={type} 
          step={step}
          value={displayParams[paramKey] !== undefined ? displayParams[paramKey] : ""} 
          onChange={(e) => handleChange(paramKey, e.target.value)}
          className="w-28 text-right bg-white border-y border-l border-slate-300 px-3 py-2 text-sm font-mono font-bold text-slate-800 outline-none transition-all"
        />
        <div className="bg-slate-100 border border-slate-300 px-3 py-2 text-[11px] font-bold text-slate-500 flex items-center select-none min-w-[50px] justify-center">
          {unit}
        </div>
      </div>
    </div>
  );

  return (
    <div className="min-h-screen bg-slate-50 flex overflow-hidden">
      <Head>
        <title>Parametrização - Nexus OS</title>
      </Head>
      {/* Mobile overlay */}
      {sidebar && (
        <div className="fixed inset-0 bg-slate-900/60 z-20 md:hidden backdrop-blur-sm"
          onClick={() => setSidebar(false)} />
      )}

      {/* ══ SIDEBAR ══ */}
      <Sidebar isOpen={sidebar} activeRoute="/config" />

      {/* ══ MAIN WORKSPACE ══ */}
      <main className="flex-1 flex flex-col h-screen overflow-hidden">
        
        {/* Topbar */}
        <header className="bg-white px-5 py-4 flex items-center justify-between shadow-sm z-10 shrink-0 relative">
          <div className="flex items-center gap-3">
            <button onClick={() => setSidebar(!sidebar)} className="p-2 rounded-lg bg-slate-50 hover:bg-slate-100 border border-slate-200 transition-colors">
              <svg className="w-5 h-5 text-slate-600" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 6h16M4 12h16M4 18h16" /></svg>
            </button>
            <h1 className="font-bold tracking-tight text-lg text-slate-800 hidden sm:block">
              Parametrização do Sistema
            </h1>
          </div>
          
          <div className="flex items-center gap-3">
            <span className={`flex items-center gap-1.5 text-[11px] font-bold capitalize tracking-wider px-3 py-1.5 rounded-full border ${boardOnline ? "bg-emerald-50 text-emerald-700 border-emerald-200" : "bg-red-50 text-red-700 border-red-200"}`}>
              <span className={`w-1.5 h-1.5 rounded-full ${boardOnline ? "bg-emerald-500" : "bg-red-500"}`} /> {boardOnline ? "Online" : "Offline"}
            </span>
            <span className="text-[11px] font-bold capitalize tracking-wider text-slate-600 bg-slate-100 px-3 py-1.5 rounded-full border border-slate-200">
              ID: {userRole || 'Visitante'}
            </span>
          </div>
        </header>


        {/* Action Bar */}
        <div className="bg-slate-50 border-b border-slate-200 px-6 py-3 flex justify-between items-center z-0 shrink-0">
          <p className="text-xs text-slate-500 font-medium">As alterações serão gravadas na memória NVS (Flash) do microcontrolador.</p>
          <div className="flex items-center gap-3">
            <button 
              onClick={handleSync}
              className="text-xs font-bold text-slate-600 bg-white border border-slate-300 hover:bg-slate-100 px-4 py-2 rounded-lg shadow-sm transition-all flex items-center gap-2">
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" /></svg>
              Sincronizar da Placa
            </button>
            <button 
              onClick={handleSave}
              disabled={!hasChanges}
              className={`text-xs font-bold px-5 py-2 rounded-lg shadow-sm transition-all flex items-center gap-2
                ${hasChanges ? "bg-slate-900 hover:bg-black text-white shadow-slate-900/20" : "bg-slate-200 text-slate-400 cursor-not-allowed"}`}>
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M8 7H5a2 2 0 00-2 2v9a2 2 0 002 2h14a2 2 0 002-2V9a2 2 0 00-2-2h-3m-1 4l-3 3m0 0l-3-3m3 3V4" /></svg>
              Salvar Alterações
            </button>
          </div>
        </div>

        {/* ── CONTEÚDO PRINCIPAL (SETTINGS LAYOUT) ── */}
        <div className="flex-1 overflow-hidden bg-slate-50 p-4 sm:p-6 lg:p-8">
          <div className="max-w-[1200px] mx-auto w-full h-full bg-white rounded-2xl border border-slate-200 shadow-sm flex overflow-hidden">
            
            {/* Menu Lateral de Configurações */}
            <div className="w-64 bg-slate-50/50 border-r border-slate-200 p-4 shrink-0 flex flex-col gap-1 overflow-y-auto">
              <span className="text-[10px] font-bold text-slate-400 uppercase tracking-widest px-3 mb-2 mt-2">Módulos</span>
              
              <button onClick={() => setActiveTab("geometria")} className={`w-full text-left px-4 py-3 rounded-xl text-sm font-bold transition-all flex items-center gap-3 ${activeTab === "geometria" ? "bg-white text-blue-700 shadow-sm border border-slate-200/60" : "text-slate-600 hover:bg-slate-100 border border-transparent"}`}>
                <svg className={`w-4 h-4 ${activeTab === "geometria" ? "text-blue-600" : "text-slate-400"}`} fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19 11H5m14 0a2 2 0 012 2v6a2 2 0 01-2 2H5a2 2 0 01-2-2v-6a2 2 0 012-2m14 0V9a2 2 0 00-2-2M5 11V9a2 2 0 012-2m0 0V5a2 2 0 012-2h6a2 2 0 012 2v2M7 7h10" /></svg>
                Física do Tanque
              </button>
              
              <button onClick={() => setActiveTab("sensor")} className={`w-full text-left px-4 py-3 rounded-xl text-sm font-bold transition-all flex items-center gap-3 ${activeTab === "sensor" ? "bg-white text-blue-700 shadow-sm border border-slate-200/60" : "text-slate-600 hover:bg-slate-100 border border-transparent"}`}>
                <svg className={`w-4 h-4 ${activeTab === "sensor" ? "text-blue-600" : "text-slate-400"}`} fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M13 10V3L4 14h7v7l9-11h-7z" /></svg>
                Calibração do Sensor
              </button>
              
              <button onClick={() => setActiveTab("telemetria")} className={`w-full text-left px-4 py-3 rounded-xl text-sm font-bold transition-all flex items-center gap-3 ${activeTab === "telemetria" ? "bg-white text-blue-700 shadow-sm border border-slate-200/60" : "text-slate-600 hover:bg-slate-100 border border-transparent"}`}>
                <svg className={`w-4 h-4 ${activeTab === "telemetria" ? "text-blue-600" : "text-slate-400"}`} fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071c3.904-3.905 10.236-3.905 14.141 0M1.394 9.393c5.857-5.857 15.355-5.857 21.213 0" /></svg>
                Rede & Telemetria
              </button>

              <div className="my-2 border-t border-slate-200 mx-2"></div>
              <span className="text-[10px] font-bold text-slate-400 uppercase tracking-widest px-3 mb-2 mt-2">Avançado</span>

              <button onClick={() => setActiveTab("sistema")} className={`w-full text-left px-4 py-3 rounded-xl text-sm font-bold transition-all flex items-center gap-3 ${activeTab === "sistema" ? "bg-red-50 text-red-700 shadow-sm border border-red-100" : "text-slate-600 hover:bg-slate-100 border border-transparent"}`}>
                <svg className={`w-4 h-4 ${activeTab === "sistema" ? "text-red-600" : "text-slate-400"}`} fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" /></svg>
                Manutenção
              </button>
            </div>

            {/* Painel de Formulários */}
            <div className="flex-1 p-6 md:p-10 lg:p-12 overflow-y-auto">
              
              {activeTab === "geometria" && (
                <div className="max-w-3xl mx-auto">
                  <div className="mb-8">
                    <h2 className="text-xl font-bold text-slate-800 tracking-tight mb-2">Física do Tanque</h2>
                    <p className="text-sm text-slate-500">Configure as dimensões reais e as margens de segurança do reservatório atual para que a conversão do sensor Ultrassónico para Litros seja precisa.</p>
                  </div>
                  
                  <div className="bg-white rounded-xl border border-slate-200 shadow-sm p-6 flex flex-col">
                    <ParamRow label="Capacidade Nominal" desc="Volume máximo útil que o tanque suporta." paramKey="TANK_MAX_VOLUME" unit="Litros" />
                    <ParamRow label="Margem de Segurança" desc="Distância de bloqueio do nível máximo/mínimo para proteção das bombas." paramKey="TANK_SAFE_MARGIN" unit="Litros" />
                    <ParamRow label="Distância Vazio (Tara)" desc="Medida do sensor até ao fundo quando o tanque tem 0 Litros." paramKey="TANK_HEIGHT_EMPTY" unit="cm" />
                    <ParamRow label="Distância Cheio (Topo)" desc="Medida do sensor até à água quando o tanque está a 100%." paramKey="TANK_HEIGHT_FULL" unit="cm" />
                    <ParamRow label="Ponto de Transbordo" desc="Medida limite. Abaixo deste valor o sistema acusa erro de leitura do sensor." paramKey="TANK_MAX_DIST" unit="cm" />
                  </div>
                </div>
              )}

              {activeTab === "sensor" && (
                <div className="max-w-3xl mx-auto">
                  <div className="mb-8">
                    <h2 className="text-xl font-bold text-slate-800 tracking-tight mb-2">Calibração do Sensor Ultrassónico</h2>
                    <p className="text-sm text-slate-500">Ajuste os filtros de suavização para evitar que as ondas da água ou respingos prejudiquem a estabilidade da leitura (Slew Rate e Média Móvel).</p>
                  </div>
                  
                  <div className="bg-white rounded-xl border border-slate-200 shadow-sm p-6 flex flex-col">
                    <ParamRow label="Amostras (Filtro EMA)" desc="Quantidade de leituras usadas para calcular a média de estabilização." paramKey="SENSOR_SAMPLES" unit="qnt" step="1" />
                    <ParamRow label="Delay do Eco" desc="Tempo de espera para o som dissipar dentro do tubo." paramKey="SENSOR_ECHO_DELAY_MS" unit="ms" step="10" />
                    <ParamRow label="Limite de Salto (Slew Rate)" desc="Variação brusca máxima permitida para rejeitar leituras anómalas." paramKey="SENSOR_MAX_SLEW_RATE_CM_S" unit="cm/s" />
                    <ParamRow label="Taxa de Atualização" desc="Tempo entre disparos físicos do pino Trigger." paramKey="SENSOR_READ_INTERVAL" unit="ms" step="10" />
                  </div>
                </div>
              )}

              {activeTab === "telemetria" && (
                <div className="max-w-3xl mx-auto">
                  <div className="mb-8">
                    <h2 className="text-xl font-bold text-slate-800 tracking-tight mb-2">Comunicação de Rede (MQTT)</h2>
                    <p className="text-sm text-slate-500">Gira a frequência com que a placa envia atualizações para o Dashboard para evitar spam na rede Wi-Fi e bloqueios no HiveMQ.</p>
                  </div>
                  
                  <h3 className="text-xs font-bold text-slate-400 uppercase tracking-widest mb-3">Tanque em Repouso (IDLE)</h3>
                  <div className="bg-white rounded-xl border border-slate-200 shadow-sm p-6 flex flex-col mb-8">
                    <ParamRow label="Gatilho de Variação (Delta)" desc="Quanto a água precisa evaporar ou vazar para forçar um envio de rede." paramKey="TELEMETRY_IDLE_DELTA_L" unit="Litros" />
                    <ParamRow label="Heartbeat Máximo" desc="Tempo máximo admissível sem comunicar estado com o servidor." paramKey="TELEMETRY_IDLE_INTERVAL_MS" unit="ms" step="1000" />
                  </div>

                  <h3 className="text-xs font-bold text-blue-500 uppercase tracking-widest mb-3">Tanque em Execução (Bombas ON)</h3>
                  <div className="bg-blue-50/30 rounded-xl border border-blue-100 shadow-sm p-6 flex flex-col">
                    <ParamRow label="Gatilho de Variação (FPS)" desc="Variação mínima de líquido necessária para atualizar a animação web." paramKey="TELEMETRY_EXEC_DELTA_L" unit="Litros" />
                    <ParamRow label="Intervalo Mínimo" desc="Limite de velocidade para não congestionar a Cloud IoT." paramKey="TELEMETRY_EXEC_INTERVAL_MS" unit="ms" step="100" />
                  </div>
                </div>
              )}

              {activeTab === "sistema" && (
                <div className="max-w-3xl mx-auto">
                  <div className="mb-8">
                    <h2 className="text-xl font-bold text-red-800 tracking-tight mb-2">Manutenção e Operações Críticas</h2>
                    <p className="text-sm text-slate-600">Ações imediatas e destrutivas que afetam o funcionamento global do sistema físico e o banco de dados interno da memória NVS.</p>
                  </div>
                  
                  <div className="border border-red-200 bg-red-50 rounded-xl overflow-hidden flex flex-col">
                    
                    <div className="p-6 border-b border-red-100 flex items-center justify-between">
                      <div className="pr-6">
                        <h4 className="text-sm font-bold text-slate-800">Formatar Base de Dados de Utilizadores</h4>
                        <p className="text-xs text-slate-600 mt-1">Apaga permanentemente todos os cartões RFID locais e permissões armazenadas fisicamente no chip do ESP32.</p>
                      </div>
                      <button onClick={handleFormatNVS} className="shrink-0 bg-white border border-red-200 text-red-600 hover:bg-red-50 font-bold text-xs px-4 py-2 rounded-lg shadow-sm transition-all">
                        Limpar NVS
                      </button>
                    </div>

                    <div className="p-6 flex items-center justify-between">
                      <div className="pr-6">
                        <h4 className="text-sm font-bold text-slate-800">Forçar Paragem de Emergência</h4>
                        <p className="text-xs text-slate-600 mt-1">Interrompe instantaneamente qualquer enchimento ou esvaziamento em curso, ignorando os estados da máquina.</p>
                      </div>
                      <button onClick={handleEmergencyStop} className="shrink-0 bg-red-600 hover:bg-red-700 text-white font-bold text-xs px-4 py-2 rounded-lg shadow-md transition-all">
                        Kill Switch (STOP)
                      </button>
                    </div>

                  </div>
                </div>
              )}

            </div>
          </div>
        </div>
      </main>
    </div>
  );
}

export async function getServerSideProps(context) {
  const cookies = context.req.headers.cookie || '';
  const match = cookies.match(/nexus_role=([^;]+)/);
  const userRole = match ? match[1] : null;

  // Bloqueio de Servidor: Apenas gestor entra
  if (userRole !== 'gestor') {
    return {
      redirect: {
        destination: userRole ? '/dashboard' : '/',
        permanent: false,
      },
    };
  }

  return { props: {} };
}