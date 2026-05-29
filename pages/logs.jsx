import { useState } from "react";
import { useTankStore } from "../store/useTankStore";
import pool from "../lib/db"; 
import { useEffect } from "react"; // Certifique-se de que useEffect está importado no topo

// --- SSR: OBTENÇÃO DE DADOS NO SERVIDOR ---
export async function getServerSideProps() {
  try {
    const result = await pool.query(`
      SELECT * FROM logs ORDER BY ts_recebido_servidor DESC LIMIT 1000
    `);

    const rawLogs = result.rows;

    const logs = rawLogs.map(row => ({
      ...row,
      id: row.id,
      ts_recebido_placa: row.ts_recebido_placa?.toISOString() || null,
      ts_inicio_execucao: row.ts_inicio_execucao?.toISOString() || null,
      ts_fim_execucao: row.ts_fim_execucao?.toISOString() || null,
      ts_recebido_servidor: row.ts_recebido_servidor?.toISOString() || null,
    }));

    // Retorna APENAS a carga de dados. A matemática das métricas será feita no cliente
    // para que reaja perfeitamente aos filtros.
    return {
      props: { initialLogs: logs },
    };
  } catch (error) {
    console.error("Erro SSR ao buscar logs:", error);
    return { props: { initialLogs: [] } };
  }
}

const NAV = [
  { name: "Painel de Telemetria", active: false, href: "/dashboard", icon: "M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zM14 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zM14 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z" },
  { name: "Gestão de Usuários",   active: false, href: "/usuarios", icon: "M12 4.354a4 4 0 110 5.292M15 21H3v-1a6 6 0 0112 0v1zm0 0h6v-1a6 6 0 00-9-5.197M13 7a4 4 0 11-8 0 4 4 0 018 0z" },
  { name: "Logs & Auditoria",     active: true,  href: "/logs",     icon: "M9 17v-2m3 2v-4m3 4v-6m2 10H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" },
  { name: "Parametrização",       active: false, href: "/config",   icon: "M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z M15 12a3 3 0 11-6 0 3 3 0 016 0z" },
];

export default function LogsBI({ initialLogs = [] }) {
  const [sidebar, setSidebar] = useState(true);
  const boardOnline = useTankStore((state) => state.boardOnline);
  
  // Estados dos Filtros Locais
  const [filtroStatus, setFiltroStatus] = useState("TODOS");
  const [filtroTanque, setFiltroTanque] = useState("TODOS");
  const [filtroPeriodo, setFiltroPeriodo] = useState("7_DIAS");
  const [filtroOperador, setFiltroOperador] = useState("TODOS");
  const [activeMetric, setActiveMetric] = useState("consumo");

  // --- NOVOS ESTADOS PARA O B.I. ---
  const [chartMode, setChartMode] = useState("media"); // "media" ou "raw"
  const [selectedBar, setSelectedBar] = useState(null); // Dados do Tooltip

  // Estados da Paginação
  const [currentPage, setCurrentPage] = useState(1);
  const [pageInput, setPageInput] = useState("1");
  const ITEMS_PER_PAGE = 20;

  // Limpa o tooltip se o usuário mudar de métrica ou aba
  useEffect(() => {
    setSelectedBar(null);
  }, [activeMetric, chartMode, filtroPeriodo, filtroTanque]);

  // Extração Dinâmica de Entidades para os Filtros
  const tanquesDisponiveis = [...new Set(initialLogs.map(l => l.tanque_id).filter(Boolean))];
  const operadoresDisponiveis = [...new Set(initialLogs.map(l => l.usuario_id).filter(Boolean))];

  // Motor de Filtragem
  const logsFiltrados = initialLogs.filter(log => {
    if (filtroStatus !== "TODOS" && log.status !== filtroStatus) return false;
    if (filtroTanque !== "TODOS" && log.tanque_id !== filtroTanque) return false;
    if (filtroOperador !== "TODOS" && log.usuario_id !== filtroOperador) return false;
    
    if (filtroPeriodo !== "TODOS" && log.ts_fim_execucao) {
      const dataLog = new Date(log.ts_fim_execucao);
      const agora = new Date();
      if (filtroPeriodo === "7_DIAS") {
        const seteDiasAtras = new Date(agora.setDate(agora.getDate() - 7));
        if (dataLog < seteDiasAtras) return false;
      } else if (filtroPeriodo === "ESTE_MES") {
        if (dataLog.getMonth() !== agora.getMonth() || dataLog.getFullYear() !== agora.getFullYear()) return false;
      } else if (filtroPeriodo === "MES_PASSADO") {
        let mesAnt = agora.getMonth() - 1;
        let ano = agora.getFullYear();
        if (mesAnt < 0) { mesAnt = 11; ano--; }
        if (dataLog.getMonth() !== mesAnt || dataLog.getFullYear() !== ano) return false;
      }
    }
    return true;
  });

  // Lógica de divisão dos logs para a Paginação
  const totalPages = Math.ceil(logsFiltrados.length / ITEMS_PER_PAGE) || 1;
  const paginatedLogs = logsFiltrados.slice((currentPage - 1) * ITEMS_PER_PAGE, currentPage * ITEMS_PER_PAGE);

  // Função para mudar de página sem precisar de useEffect
  const handlePageChange = (newPage) => {
    const validPage = Math.max(1, Math.min(newPage, totalPages));
    setCurrentPage(validPage);
    setPageInput(validPage.toString());
  };

  // --- MATEMÁTICA DO B.I. (Dinâmica baseada no filtro atual) ---
  let somaDuracao = 0, somaLatencia = 0, somaConsumo = 0;
  let sucessos = 0, cancelados = 0, timeouts = 0, outrosErros = 0;
  let webCount = 0, rfidCount = 0;

  logsFiltrados.forEach(log => {
    somaDuracao += (log.duracao_ms || 0);
    somaConsumo += Math.abs((log.volume_final || 0) - (log.volume_inicial || 0));

    if (log.ts_fim_execucao && log.ts_recebido_servidor) {
      somaLatencia += (new Date(log.ts_recebido_servidor).getTime() - new Date(log.ts_fim_execucao).getTime());
    }

    if (log.status === 'SUCESSO') sucessos++;
    else if (log.status === 'CANCELADO') cancelados++;
    else if (log.status === 'TIMEOUT') timeouts++;
    else outrosErros++;

    if (log.origem_comando === "WEB_DASHBOARD") webCount++;
    else if (log.origem_comando === "LOCAL_RFID") rfidCount++;
  });

  const total = logsFiltrados.length > 0 ? logsFiltrados.length : 1;
  const metrics = {
    tempoMedioMs: somaDuracao / total,
    latenciaMediaMs: somaLatencia / total,
    consumoTotalL: somaConsumo,
    taxaSucesso: (sucessos / total) * 100,
    falhasPercent: ((cancelados + timeouts + outrosErros) / total) * 100,
    contagem: { sucessos, cancelados, timeouts, erros: outrosErros }
  };

  const totalOrigem = webCount + rfidCount || 1;
  const webPct = ((webCount / totalOrigem) * 100).toFixed(0);
  const rfidPct = ((rfidCount / totalOrigem) * 100).toFixed(0);

  // --- FUNÇÕES DE FORMATAÇÃO E EXPORTAÇÃO ---
  const formatarData = (isoString) => {
    if (!isoString) return "---";
    return new Date(isoString).toLocaleString('pt-BR', { dateStyle: 'short', timeStyle: 'medium' });
  };

  const formatarDataCurta = (isoString) => {
    if (!isoString) return "---";
    const d = new Date(isoString);
    return `${d.getDate().toString().padStart(2,'0')}/${(d.getMonth()+1).toString().padStart(2,'0')} ${d.getHours().toString().padStart(2,'0')}:${d.getMinutes().toString().padStart(2,'0')}`;
  };

  const formatarDuracao = (ms) => {
    if (!ms) return "0s";
    const segs = Math.floor(ms / 1000);
    const m = Math.floor(segs / 60);
    const s = segs % 60;
    return m > 0 ? `${m}m ${s}s` : `${s}s`;
  };
  
  const formatarLatencia = (ms) => {
    if (ms == null || ms === 0) return "---";
    return ms < 1000 ? `${Math.round(ms)}ms` : `${(ms/1000).toFixed(1)}s`;
  };

  const formatarDelayLinha = (fimPlaca, recServidor) => {
    if (!fimPlaca || !recServidor) return "---";
    return formatarLatencia(new Date(recServidor).getTime() - new Date(fimPlaca).getTime());
  };

  const handleExportCSV = () => {
    if (logsFiltrados.length === 0) return alert("Nenhum dado para exportar.");
    const headers = ["ID", "Tanque", "Operador", "Acao", "Origem", "Vol Inicial", "Vol Final", "Alvo", "Duracao(ms)", "Status", "Timestamp_Placa"];
    const rows = logsFiltrados.map(log => [
      log.id, log.tanque_id, log.usuario_id, log.tipo_operacao, log.origem_comando, 
      log.volume_inicial, log.volume_final, log.volume_alvo, log.duracao_ms, 
      log.status, log.ts_fim_execucao
    ]);
    const csvContent = "data:text/csv;charset=utf-8," + [headers.join(","), ...rows.map(e => e.join(","))].join("\n");
    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", `nexus_export_${new Date().getTime()}.csv`);
    document.body.appendChild(link); link.click(); document.body.removeChild(link);
  };

  // --- MOTOR GRÁFICO DINÂMICO SVG ---
  // --- MOTOR GRÁFICO DINÂMICO SVG ---
  // --- MOTOR GRÁFICO DINÂMICO SVG (Com Switch e Interatividade) ---
  const renderDynamicChart = () => {
    if (logsFiltrados.length === 0) {
      return (
        <svg viewBox="0 0 500 220" className="w-full h-full overflow-visible font-mono text-[11px] fill-slate-400">
          <text x="250" y="110" textAnchor="middle">Sem dados para os filtros selecionados</text>
        </svg>
      );
    }

    const chartData = [...logsFiltrados].reverse();
    const dataInicial = formatarDataCurta(chartData[0].ts_fim_execucao);
    const dataFinal = formatarDataCurta(chartData[chartData.length - 1].ts_fim_execucao);

    // Função para calcular clique e posicionar o Tooltip
    const handleBarClick = (e, val, titleStr, timeStr) => {
      e.stopPropagation(); // Impede que o clique feche o tooltip imediatamente
      const svgRect = e.currentTarget.closest('svg').getBoundingClientRect();
      const x = e.clientX - svgRect.left;
      const y = e.clientY - svgRect.top;
      setSelectedBar({ value: val, title: titleStr, time: timeStr, x, y });
    };

    const calcularMediaMovel = (dados, periodo = 4) => {
      return dados.map((val, i, arr) => {
        if (i === 0) return val;
        const inicio = Math.max(0, i - periodo + 1);
        const sub = arr.slice(inicio, i + 1);
        return sub.reduce((a, b) => a + b, 0) / sub.length;
      });
    };

    const normalize = (val, max) => max === 0 ? 0 : (val / max) * 150;
    const generatePath = (dataArr, maxVal) => {
      if (dataArr.length === 1) return `M 50 ${170 - normalize(dataArr[0], maxVal)} L 490 ${170 - normalize(dataArr[0], maxVal)}`;
      const step = 440 / (dataArr.length - 1);
      return dataArr.map((v, i) => `${i===0?'M':'L'} ${50 + (i*step)} ${170 - normalize(v, Math.max(maxVal, 1))}`).join(" ");
    };

    const consumoArr = []; const latenciaArr = []; const tempoArr = [];
    const eficSucc = []; let accS = 0; const eficCanc = []; let accC_ef = 0; const eficFail = []; let accF = 0;

    chartData.forEach((l, i) => {
      consumoArr.push(l.volume_final || 0); 
      latenciaArr.push((l.ts_fim_execucao && l.ts_recebido_servidor) ? new Date(l.ts_recebido_servidor).getTime() - new Date(l.ts_fim_execucao).getTime() : 0);
      tempoArr.push(l.duracao_ms || 0);
      if (l.status === 'SUCESSO') accS++; else if (l.status === 'CANCELADO') accC_ef++; else accF++;
      eficSucc.push((accS / (i + 1)) * 100); eficCanc.push((accC_ef / (i + 1)) * 100); eficFail.push((accF / (i + 1)) * 100);
    });

    const EixosEGrades = ({ maxLabel, midLabel, bottomLabel }) => (
      <>
        <text x="35" y="24" textAnchor="end">{maxLabel}</text>
        <text x="35" y="95" textAnchor="end">{midLabel}</text>
        <text x="35" y="174" textAnchor="end">{bottomLabel}</text>
        <line x1="45" y1="20" x2="490" y2="20" stroke="#f1f5f9" strokeWidth="1" strokeDasharray="4 4" />
        <line x1="45" y1="95" x2="490" y2="95" stroke="#f1f5f9" strokeWidth="1" strokeDasharray="4 4" />
        <line x1="45" y1="170" x2="490" y2="170" stroke="#cbd5e1" strokeWidth="1.5" strokeLinecap="round" />
        <text x="50" y="195" textAnchor="middle">{dataInicial}</text>
        <text x="490" y="195" textAnchor="end" fontWeight="bold" fill="#64748b">{dataFinal}</text>
      </>
    );

    const barStep = 440 / (chartData.length || 1);
    const barW = Math.max(4, barStep - 4);

    switch (activeMetric) {
      case "tempo":
        const maxTempo = Math.max(...tempoArr, 1000);
        return (
          <svg viewBox="0 0 500 220" className="w-full h-full overflow-visible font-mono text-[9px] fill-slate-400">
            <EixosEGrades maxLabel={formatarDuracao(maxTempo)} midLabel={formatarDuracao(maxTempo/2)} bottomLabel="0s" />
            {tempoArr.map((v, i) => (
              <rect key={i} x={50 + (i*barStep) + 2} y={170 - normalize(v, maxTempo)} width={barW} height={normalize(v, maxTempo)} fill="#cbd5e1" 
                    className="hover:fill-blue-500 transition-colors cursor-pointer"
                    onClick={(e) => handleBarClick(e, formatarDuracao(v), "Duração da Tarefa", formatarData(chartData[i].ts_fim_execucao))} />
            ))}
          </svg>
        );

      case "latencia":
        const maxLat = Math.max(...latenciaArr, 100);
        const isAttentionLat = metrics.latenciaMediaMs > 5000;
        const colorLat = isAttentionLat ? "#f59e0b" : "#10b981"; 

        if (chartMode === "raw") {
          return (
            <svg viewBox="0 0 500 220" className="w-full h-full overflow-visible font-mono text-[9px] fill-slate-400">
              <EixosEGrades maxLabel={formatarLatencia(maxLat)} midLabel={formatarLatencia(maxLat/2)} bottomLabel="0ms" />
              {latenciaArr.map((v, i) => (
                <rect key={i} x={50 + (i*barStep) + 2} y={170 - normalize(v, maxLat)} width={barW} height={normalize(v, maxLat)} fill={colorLat} opacity="0.7"
                      className="hover:opacity-100 transition-all cursor-pointer"
                      onClick={(e) => handleBarClick(e, formatarLatencia(v), "Delay/Latência", formatarData(chartData[i].ts_fim_execucao))} />
              ))}
            </svg>
          );
        }

        // Média Móvel (Linha + Sombra)
        const latenciaSMA = calcularMediaMovel(latenciaArr, Math.max(3, Math.floor(latenciaArr.length / 10)));
        const pPathSMALat = generatePath(latenciaSMA, maxLat);
        return (
          <svg viewBox="0 0 500 220" className="w-full h-full overflow-visible font-mono text-[9px] fill-slate-400">
            <defs><linearGradient id="lGrad" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stopColor={colorLat} stopOpacity="0.25"/><stop offset="100%" stopColor={colorLat} stopOpacity="0.0"/></linearGradient></defs>
            <EixosEGrades maxLabel={formatarLatencia(maxLat)} midLabel={formatarLatencia(maxLat/2)} bottomLabel="0ms" />
            {chartData.length > 1 && <path d={`${pPathSMALat} L 490 170 L 50 170 Z`} fill="url(#lGrad)" />}
            <path d={pPathSMALat} fill="none" stroke={colorLat} strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" />
            <circle cx="490" cy={170 - normalize(latenciaSMA[latenciaSMA.length-1], maxLat)} r="4.5" fill={colorLat} stroke="white" strokeWidth="1.5" />
          </svg>
        );

      case "eficiencia":
        return (
          <svg viewBox="0 0 500 220" className="w-full h-full overflow-visible font-mono text-[9px] fill-slate-400">
            <EixosEGrades maxLabel="100%" midLabel="50%" bottomLabel="0%" />
            <path d={generatePath(eficSucc, 100)} fill="none" stroke="#10b981" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" />
            <path d={generatePath(eficCanc, 100)} fill="none" stroke="#fbbf24" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" strokeDasharray="4 4" />
            <path d={generatePath(eficFail, 100)} fill="none" stroke="#ef4444" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" strokeDasharray="4 4" />
          </svg>
        );

      case "consumo":
      default:
        const maxCons = Math.max(...consumoArr, 100);
        
        if (chartMode === "raw") {
          return (
            <svg viewBox="0 0 500 220" className="w-full h-full overflow-visible font-mono text-[9px] fill-slate-400">
              <EixosEGrades maxLabel={`${maxCons.toFixed(0)}L`} midLabel={`${(maxCons/2).toFixed(0)}L`} bottomLabel="0L" />
              {consumoArr.map((v, i) => (
                <rect key={i} x={50 + (i*barStep) + 2} y={170 - normalize(v, maxCons)} width={barW} height={normalize(v, maxCons)} fill="#3b82f6" opacity="0.8"
                      className="hover:opacity-100 transition-all cursor-pointer"
                      onClick={(e) => handleBarClick(e, `${v.toFixed(1)} L`, "Nível do Tanque", formatarData(chartData[i].ts_fim_execucao))} />
              ))}
            </svg>
          );
        }

        // Média Móvel (Linha + Sombra)
        const consumoSMA = calcularMediaMovel(consumoArr, Math.max(3, Math.floor(consumoArr.length / 8)));
        const pPathSMACons = generatePath(consumoSMA, maxCons);
        return (
          <svg viewBox="0 0 500 220" className="w-full h-full overflow-visible font-mono text-[9px] fill-slate-400">
            <defs><linearGradient id="cGrad" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stopColor="#3b82f6" stopOpacity="0.25"/><stop offset="100%" stopColor="#3b82f6" stopOpacity="0.0"/></linearGradient></defs>
            <EixosEGrades maxLabel={`${maxCons.toFixed(0)}L`} midLabel={`${(maxCons/2).toFixed(0)}L`} bottomLabel="0L" />
            {chartData.length > 1 && <path d={`${pPathSMACons} L 490 170 L 50 170 Z`} fill="url(#cGrad)" />}
            <path d={pPathSMACons} fill="none" stroke="#2563eb" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" />
            <circle cx="490" cy={170 - normalize(consumoSMA[consumoSMA.length-1], maxCons)} r="4" fill="#2563eb" stroke="white" strokeWidth="1.5" />
          </svg>
        );
    }
  };

  const getChartHeaders = () => {
    switch (activeMetric) {
      case "tempo": return { title: "Evolução do Tempo de Execução", desc: "Duração dos ciclos logados nos filtros selecionados" };
      case "latencia": return { title: "Estabilidade de Rede e Sincronização", desc: "Atraso (delay) entre finalização na placa e o registro na base de dados" };
      case "eficiencia": return { title: "Curva de Eficiência Operacional", desc: "Taxa acumulada de tarefas concluídas com sucesso vs falhas/abortos" };
      case "consumo": default: return { title: "Nível do Tanque ao Longo do Tempo", desc: "Atividade de litros movimentados (enchimento/esvaziamento) nos filtros atuais" };
    }
  };

  return (
    <div className="min-h-screen bg-slate-50 flex overflow-hidden">
      
      {/* ══ SIDEBAR ══ */}
      <aside className={`fixed md:relative z-30 inset-y-0 left-0 h-screen bg-slate-900 border-r border-slate-800 transition-all duration-300 overflow-hidden ${sidebar ? "w-64 translate-x-0" : "w-64 -translate-x-full md:w-0 md:translate-x-0"}`}>
        <div className="w-64 h-full flex flex-col">
          <div className="p-5 border-b border-slate-800 flex items-center gap-3 shrink-0">
            <div className="w-8 h-8 bg-blue-600 rounded-lg flex items-center justify-center shadow-md shadow-blue-900/40">
              <svg className="w-5 h-5 text-white" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19.428 15.428a2 2 0 00-1.022-.547l-2.387-.477a6 6 0 00-3.86.517l-.318.158a6 6 0 01-3.86.517L6.05 15.21a2 2 0 00-1.806.547M8 4h8l-1 1v5.172a2 2 0 00.586 1.414l5 5c1.26 1.26.367 3.414-1.415 3.414H4.828c-1.782 0-2.674-2.154-1.414-3.414l5-5A2 2 0 009 10.172V5L8 4z" /></svg>
            </div>
            <div><span className="text-white font-bold tracking-wide text-sm block">Nexus Open-Logic</span><span className="text-slate-500 text-[10px]">v2.4.1 — BI Platform</span></div>
          </div>
          <nav className="flex-1 p-3 space-y-1 text-sm font-medium overflow-y-auto">
            {NAV.map((item) => (
              <a key={item.name} href={item.href} className={`flex items-center gap-3 px-4 py-2.5 rounded-lg transition-colors ${item.active ? "bg-blue-600/10 text-blue-400 border border-blue-500/20" : "text-slate-400 hover:bg-slate-800 hover:text-white"}`}>
                <svg className="w-5 h-5 shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d={item.icon} /></svg>
                {item.name}
              </a>
            ))}
          </nav>
        </div>
      </aside>

      {/* ══ MAIN WORKSPACE ══ */}
      <main className="flex-1 flex flex-col h-screen overflow-hidden">
        
        {/* Topbar */}
        <header className="bg-white px-5 py-3 flex items-center justify-between shadow-sm z-10 shrink-0 relative">
          <div className="flex items-center gap-3">
            <button onClick={() => setSidebar(!sidebar)} className="p-2 rounded-lg bg-slate-50 border border-slate-200"><svg className="w-5 h-5 text-slate-600" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 6h16M4 12h16M4 18h16" /></svg></button>
            <span className="font-bold tracking-tight text-base text-slate-700">Business Intelligence & Auditoria</span>
          </div>
          <span className={`flex items-center gap-1.5 text-xs font-semibold px-3 py-1.5 rounded-full border ${boardOnline ? "bg-emerald-50 text-emerald-700 border-emerald-200" : "bg-red-50 text-red-700 border-red-200"}`}>
            <span className={`w-1.5 h-1.5 rounded-full ${boardOnline ? "bg-emerald-500" : "bg-red-500"}`} /> {boardOnline ? "Placa Online" : "Placa Offline"}
          </span>
        </header>

        {/* ── BARRA DE CONTROLE E FILTROS DINÂMICOS ── */}
        <div className="bg-white rounded-xl border border-slate-200 shadow-sm p-3 flex flex-wrap gap-4 items-center justify-between z-10 relative">
          <div className="flex items-center gap-4">
            <div className="flex items-center gap-2 pl-2">
              <div className="p-1.5 bg-slate-800 text-white rounded-md shadow-sm">
                <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" /></svg>
              </div>
              <span className="text-[11px] font-bold text-slate-800 uppercase tracking-widest hidden sm:block">Parâmetros</span>
            </div>
            
            <div className="h-6 w-px bg-slate-200 hidden md:block"></div>
            
            <div className="flex items-center gap-2">
              <div className="relative group">
                <select value={filtroTanque} onChange={(e) => setFiltroTanque(e.target.value)} className="appearance-none bg-slate-50 group-hover:bg-slate-100 border border-slate-200 text-slate-600 text-xs font-semibold rounded-lg pl-3 pr-8 py-2 outline-none focus:border-blue-500 focus:ring-1 focus:ring-blue-500 transition-all cursor-pointer shadow-sm">
                  <option value="TODOS">Todos os Tanques</option>
                  {tanquesDisponiveis.map(t => <option key={t} value={t}>{t}</option>)}
                </select>
                <svg className="w-3 h-3 text-slate-400 absolute right-3 top-1/2 -translate-y-1/2 pointer-events-none" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19 9l-7 7-7-7" /></svg>
              </div>

              <div className="relative group">
                <select value={filtroPeriodo} onChange={(e) => setFiltroPeriodo(e.target.value)} className="appearance-none bg-slate-50 group-hover:bg-slate-100 border border-slate-200 text-slate-600 text-xs font-semibold rounded-lg pl-3 pr-8 py-2 outline-none focus:border-blue-500 focus:ring-1 focus:ring-blue-500 transition-all cursor-pointer shadow-sm">
                  <option value="TODOS">Todo o Período</option>
                  <option value="7_DIAS">Últimos 7 dias</option>
                  <option value="ESTE_MES">Este Mês</option>
                  <option value="MES_PASSADO">Mês Passado</option>
                </select>
                <svg className="w-3 h-3 text-slate-400 absolute right-3 top-1/2 -translate-y-1/2 pointer-events-none" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19 9l-7 7-7-7" /></svg>
              </div>

              <div className="relative group">
                <select value={filtroOperador} onChange={(e) => setFiltroOperador(e.target.value)} className="appearance-none bg-slate-50 group-hover:bg-slate-100 border border-slate-200 text-slate-600 text-xs font-semibold rounded-lg pl-3 pr-8 py-2 outline-none focus:border-blue-500 focus:ring-1 focus:ring-blue-500 transition-all cursor-pointer shadow-sm">
                  <option value="TODOS">Todos os Operadores</option>
                  {operadoresDisponiveis.map(op => <option key={op} value={op}>{op}</option>)}
                </select>
                <svg className="w-3 h-3 text-slate-400 absolute right-3 top-1/2 -translate-y-1/2 pointer-events-none" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19 9l-7 7-7-7" /></svg>
              </div>
            </div>
          </div>

          <div className="pr-1">
            <button onClick={handleExportCSV} className="flex items-center gap-2 px-4 py-2 bg-slate-800 hover:bg-slate-900 text-white text-xs font-bold rounded-lg shadow-md hover:shadow-lg transition-all active:scale-95 group">
              <svg className="w-4 h-4 text-slate-300 group-hover:text-white transition-colors" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4" /></svg>
              Exportar <span className="hidden sm:inline">Relatório (.csv)</span>
            </button>
          </div>
        </div>

        {/* ── CONTEÚDO PRINCIPAL ── */}
        <div className="flex-1 overflow-y-auto bg-slate-50/50">
          <div className="p-5 max-w-[1200px] mx-auto w-full flex flex-col gap-5">
            
            {/* ── METRIC CARDS ROW ── */}
            <div className="grid grid-cols-4 gap-4">
              
              <button onClick={() => setActiveMetric("tempo")} className={`text-left rounded-xl border shadow-sm p-4 flex flex-col justify-between transition-all duration-200 hover:-translate-y-0.5 ${activeMetric === "tempo" ? "bg-blue-50/50 border-blue-400 ring-1 ring-blue-400" : "bg-white border-slate-200 hover:border-slate-300"}`}>
                <div>
                  <span className={`text-[10px] font-bold uppercase tracking-widest block ${activeMetric === "tempo" ? "text-blue-600" : "text-slate-400"}`}>Tempo Médio</span>
                  <p className="text-2xl font-bold text-slate-800 font-mono mt-1">{logsFiltrados.length ? formatarDuracao(metrics.tempoMedioMs) : "---"}</p>
                </div>
                <p className="text-[10px] text-slate-400 mt-3">Tempo médio do filtro atual</p>
              </button>

              {/* Latência (Com Cores Reativas) */}
              {/* Latência (Com Cores Reativas e Barra Proporcional) */}
              <button 
                onClick={() => setActiveMetric("latencia")} 
                className={`text-left rounded-xl border shadow-sm p-4 flex flex-col justify-between transition-all duration-200 hover:-translate-y-0.5 
                  ${activeMetric === "latencia" 
                    ? (metrics.latenciaMediaMs > 5000 ? "bg-amber-50/50 border-amber-400 ring-1 ring-amber-400" : "bg-emerald-50/50 border-emerald-400 ring-1 ring-emerald-400") 
                    : "bg-white border-slate-200 hover:border-slate-300"}`}>
                <div>
                  <span className={`text-[10px] font-bold uppercase tracking-widest block ${activeMetric === "latencia" ? (metrics.latenciaMediaMs > 5000 ? "text-amber-600" : "text-emerald-600") : "text-slate-400"}`}>Delay de Sync</span>
                  <p className={`text-2xl font-bold font-mono mt-1 ${metrics.latenciaMediaMs > 5000 ? "text-amber-600" : "text-emerald-600"}`}>
                    {logsFiltrados.length ? formatarLatencia(metrics.latenciaMediaMs) : "---"}
                  </p>
                </div>
                <div className="w-full h-1 bg-slate-100 rounded-full mt-3 overflow-hidden">
                  {/* A mágica acontece aqui: A barra cresce proporcionalmente até o limite de 5000ms */}
                  <div className={`h-full transition-all ${metrics.latenciaMediaMs > 5000 ? "bg-amber-500" : "bg-emerald-500"}`} 
                       style={{ width: logsFiltrados.length ? `${Math.min((metrics.latenciaMediaMs / 5000) * 100, 100)}%` : '0%' }} />
                </div>
                <p className="text-[10px] text-slate-400 mt-1">
                  Estabilidade: <span className={`font-bold ${metrics.latenciaMediaMs > 5000 ? 'text-amber-600' : 'text-emerald-600'}`}>
                    {logsFiltrados.length ? (metrics.latenciaMediaMs > 5000 ? 'Atenção' : 'Alta') : "---"}
                  </span>
                </p>
              </button>

              <button onClick={() => setActiveMetric("consumo")} className={`text-left rounded-xl border shadow-sm p-4 flex flex-col justify-between transition-all duration-200 hover:-translate-y-0.5 ${activeMetric === "consumo" ? "bg-blue-50/50 border-blue-400 ring-1 ring-blue-400" : "bg-white border-slate-200 hover:border-slate-300"}`}>
                <div>
                  <span className={`text-[10px] font-bold uppercase tracking-widest block ${activeMetric === "consumo" ? "text-blue-600" : "text-slate-400"}`}>Volume Consumido</span>
                  <p className="text-2xl font-bold text-blue-600 font-mono mt-1">{logsFiltrados.length ? metrics.consumoTotalL.toFixed(1) : "0.0"} L</p>
                </div>
                <p className="text-[10px] text-slate-400 mt-3">Total do período selecionado</p>
              </button>

              <button onClick={() => setActiveMetric("eficiencia")} className={`text-left rounded-xl border shadow-sm p-4 flex flex-col justify-between transition-all duration-200 hover:-translate-y-0.5 ${activeMetric === "eficiencia" ? "bg-blue-50/50 border-blue-400 ring-1 ring-blue-400" : "bg-white border-slate-200 hover:border-slate-300"}`}>
                <div>
                  <span className={`text-[10px] font-bold uppercase tracking-widest block ${activeMetric === "eficiencia" ? "text-blue-600" : "text-slate-400"}`}>Taxa de Sucesso</span>
                  <p className="text-2xl font-bold text-emerald-700 font-mono mt-1">{logsFiltrados.length ? metrics.taxaSucesso.toFixed(1) : "0.0"}%</p>
                </div>
                <div className="flex items-center gap-1.5 mt-3">
                  <span className="w-2 h-2 rounded-full bg-emerald-500" title="Sucessos" />
                  <span className="w-2 h-2 rounded-full bg-amber-400" title="Cancelados" />
                  <span className="w-2 h-2 rounded-full bg-red-500" title="Timeouts/Erros" />
                  <span className="text-[10px] text-slate-400 ml-auto">{logsFiltrados.length ? metrics.falhasPercent.toFixed(1) : "0.0"}% falhas/abortos</span>
                </div>
              </button>
            </div>

            {/* ── CHARTS SECTION ── */}
            {/* ── CHARTS SECTION ── */}
            <div className="grid grid-cols-3 gap-4">
              <div className="col-span-2 bg-white rounded-xl border border-slate-200 shadow-sm p-5 flex flex-col">
                
                {/* CABEÇALHO DO GRÁFICO (Textos, Switch e Total) */}
                <div className="mb-4 flex items-start justify-between">
                  <div>
                    <h3 className="text-sm font-bold text-slate-800">{getChartHeaders().title}</h3>
                    <p className="text-[11px] text-slate-400">{getChartHeaders().desc}</p>
                  </div>
                  
                  <div className="flex flex-col items-end gap-2">

                    {/* Botão Switch Clean */}
                    {(activeMetric === "consumo" || activeMetric === "latencia") && (
                      <div className="flex bg-slate-100 p-0.5 rounded-lg border border-slate-200">
                        <button onClick={() => {setChartMode("media"); setSelectedBar(null);}} 
                                className={`px-3 py-1 text-[10px] font-bold rounded-md transition-all ${chartMode === "media" ? "bg-white shadow-sm text-slate-800" : "text-slate-400 hover:text-slate-600"}`}>
                          Média Suavizada
                        </button>
                        <button onClick={() => {setChartMode("raw"); setSelectedBar(null);}} 
                                className={`px-3 py-1 text-[10px] font-bold rounded-md transition-all ${chartMode === "raw" ? "bg-white shadow-sm text-slate-800" : "text-slate-400 hover:text-slate-600"}`}>
                          Dados Brutos (Barras)
                        </button>
                      </div>
                    )}
                  </div>
                </div>
                
                {/* CONTAINER DO SVG E TOOLTIP */}
                <div className="flex-1 w-full min-h-[220px] bg-slate-50/50 rounded-lg border border-slate-100 p-2 flex flex-col justify-between relative" onClick={() => setSelectedBar(null)}>
                    {/* Texto de Total Movimentado Realocado (Bem implementado em HTML) */}
                    {/* Texto de Total Movimentado fixo no canto superior direito */}
                    {activeMetric === "consumo" && (
                      <div className="absolute top-2 right-2 text-blue-700 text-[10px] font-bold drop-shadow-sm">
                        TOTAL MOVIMENTADO: {metrics.consumoTotalL.toFixed(1)} L
                      </div>
                    )}
                  {/* Tooltip Interativo Absoluto */}
                  {selectedBar && (
                    <div className="absolute z-20 bg-slate-800 text-white px-3 py-2 rounded-lg shadow-xl pointer-events-none transform -translate-x-1/2 -translate-y-full flex flex-col gap-1 border border-slate-600 transition-all" 
                         style={{ left: selectedBar.x, top: selectedBar.y - 10 }}>
                      <span className="text-[9px] font-bold text-blue-300 uppercase tracking-wider">{selectedBar.title}</span>
                      <span className="text-sm font-mono font-bold leading-none">{selectedBar.value}</span>
                      <span className="text-[9px] text-slate-400 border-t border-slate-600 pt-1 mt-1">{selectedBar.time}</span>
                      <div className="absolute w-2 h-2 bg-slate-800 border-b border-r border-slate-600 transform rotate-45 -bottom-1 left-1/2 -translate-x-1/2"></div>
                    </div>
                  )}

                  {renderDynamicChart()}
                </div>
              </div>

              {/* Origem dos Comandos */}
              <div className="col-span-1 bg-white rounded-xl border border-slate-200 shadow-sm p-5 flex flex-col justify-between">
                <div>
                  <h3 className="text-sm font-bold text-slate-800">Origem de Comandos</h3>
                  <p className="text-[11px] text-slate-400 mb-4">Dashboard Web vs. RFID Local</p>
                </div>

                <div className="space-y-4">
                  <div>
                    <div className="flex justify-between text-xs font-semibold text-slate-600 mb-1">
                      <span className="flex items-center gap-1.5">
                        {/* Ícone de computador
                        <svg xmlns="http://www.w3.org/2000/svg" 
                            fill="none" 
                            viewBox="0 0 24 24" 
                            strokeWidth="1.5" 
                            stroke="currentColor" 
                            className="w-3.5 h-3.5 text-slate-600">
                          <path strokeLinecap="round" strokeLinejoin="round" d="M9 17.25v1.007a3 3 0 0 1-.879 2.122L7.5 21h9l-.621-.621A3 3 0 0 1 15 18.257V17.25m6-12V15a2.25 2.25 0 0 1-2.25 2.25H5.25A2.25 2.25 0 0 1 3 15V5.25m18 0A2.25 2.25 0 0 0 18.75 3H5.25A2.25 2.25 0 0 0 3 5.25m18 0V12a2.25 2.25 0 0 1-2.25 2.25H5.25A2.25 2.25 0 0 1 3 12V5.25" />
                        </svg> */}
                        DASHBOARD WEB
                      </span>
                      <span className="font-mono">{webPct}%</span>
                    </div>
                    <div className="w-full h-2 bg-slate-100 rounded-full overflow-hidden">
                      <div className="h-full bg-blue-600 rounded-full transition-all" style={{ width: `${webPct}%` }} />
                    </div>
                  </div>

                  <div>
                    <div className="flex justify-between text-xs font-semibold text-slate-600 mb-1">
                      <span className="flex items-center gap-1.5">
                        {/* Ícone de RFID 
                        <svg xmlns="http://www.w3.org/2000/svg" 
                            fill="none" 
                            viewBox="0 0 24 24" 
                            strokeWidth="1.5" 
                            stroke="currentColor" 
                            className="w-3.5 h-3.5 text-slate-600">
                          <path strokeLinecap="round" strokeLinejoin="round" d="M2.25 8.25h19.5M2.25 9h19.5m-16.5 5.25h6m-6 2.25h3m-3.75 3h15a2.25 2.25 0 0 0 2.25-2.25V6.75A2.25 2.25 0 0 0 19.5 4.5h-15a2.25 2.25 0 0 0-2.25 2.25v10.5A2.25 2.25 0 0 0 4.5 19.5Z" />
                        </svg>*/}
                        SISTEMA FISICO
                      </span>
                      <span className="font-mono">{rfidPct}%</span>
                    </div>
                    <div className="w-full h-2 bg-slate-100 rounded-full overflow-hidden">
                      <div className="h-full bg-purple-500 rounded-full transition-all" style={{ width: `${rfidPct}%` }} />
                    </div>
                  </div>
                </div>


                <div className="border-t border-slate-100 pt-4 mt-4">
                  <span className="text-[10px] font-bold uppercase tracking-widest text-slate-400 block mb-2">Total do Período</span>
                  <div className="grid grid-cols-3 gap-2 text-center">
                    <div className="bg-emerald-50 border border-emerald-100 p-2 rounded-lg">
                      <span className="text-[10px] font-bold text-emerald-700 block">{metrics.contagem.sucessos}</span>
                      <span className="text-[9px] text-emerald-600 font-medium">Sucessos</span>
                    </div>
                    <div className="bg-amber-50 border border-amber-100 p-2 rounded-lg">
                      <span className="text-[10px] font-bold text-amber-700 block">{metrics.contagem.cancelados}</span>
                      <span className="text-[9px] text-amber-600 font-medium">Cancelados</span>
                    </div>
                    <div className="bg-red-50 border border-red-100 p-2 rounded-lg">
                      <span className="text-[10px] font-bold text-red-700 block">{metrics.contagem.timeouts}</span>
                      <span className="text-[9px] text-red-600 font-medium">Timeout</span>
                    </div>
                  </div>
                </div>
              </div>
            </div>

            {/* ── TABELA DE LOGS ── */}
            <div className="bg-white rounded-xl border border-slate-200 shadow-sm overflow-hidden">
              <div className="p-4 border-b border-slate-100 bg-slate-50/60 flex items-center justify-between">
                <div>
                  <h3 className="text-sm font-bold text-slate-800">Livro de Registro de Auditoria (`logs`)</h3>
                  <p className="text-[11px] text-slate-400 mt-0.5">Histórico de operações persistidas via REST API</p>
                </div>
                
                <div className="flex gap-1 bg-slate-100 p-1 rounded-lg border border-slate-200 text-xs font-semibold">
                  {["TODOS", "SUCESSO", "CANCELADO", "TIMEOUT"].map((st) => (
                    <button key={st} onClick={() => setFiltroStatus(st)} className={`px-2.5 py-1 rounded-md transition-colors ${filtroStatus === st ? "bg-white text-slate-800 shadow-sm" : "text-slate-400 hover:text-slate-600"}`}>
                      {st}
                    </button>
                  ))}
                </div>
              </div>

              <div className="overflow-x-auto">
                <table className="w-full text-left text-xs border-collapse">
                  <thead>
                    <tr className="bg-slate-50/70 border-b border-slate-200 text-slate-400 font-bold uppercase tracking-wider text-[10px]">
                      <th className="p-3 pl-5">ID Operação</th>
                      <th className="p-3">Operador</th>
                      <th className="p-3">Ação</th>
                      <th className="p-3">Origem</th>
                      <th className="p-3">Métricas (Volume)</th>
                      <th className="p-3">Duração</th>
                      <th className="p-3">Latência Sync</th>
                      <th className="p-3">Timestamp (UTC)</th>
                      <th className="p-3 pr-5 text-right">Status</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-100 text-slate-700 font-medium">
                    {paginatedLogs.length === 0 ? (
                      <tr>
                        <td colSpan="9" className="p-8 text-center text-slate-400">Nenhum registro encontrado para estes filtros.</td>
                      </tr>
                    ) : (
                      paginatedLogs.map((log) => (
                        <tr key={log.id} className="hover:bg-slate-50/50 transition-colors">
                          <td className="p-3 pl-5 font-mono text-slate-400 text-[10px]" title={log.id}>{log.id.split('-')[0]}...</td>
                          <td className="p-3 font-semibold text-slate-800">{log.usuario_id}</td>
                          <td className="p-3">
                            <span className={`px-2 py-0.5 rounded text-[10px] font-bold ${log.tipo_operacao === "ENCHER" ? "bg-blue-50 text-blue-600" : "bg-orange-50 text-orange-600"}`}>
                              {log.tipo_operacao === "ENCHER" ? "↑ ENCHER" : "↓ ESVAZIAR"}
                            </span>
                          </td>
                          <td className="p-3 font-mono text-slate-500 text-[11px]">{log.origem_comando}</td>
                          <td className="p-3">
                            <span className="font-mono text-slate-600">{log.volume_inicial?.toFixed(1) || "0.0"}L</span>
                            <span className="text-slate-300 mx-1.5">➔</span>
                            <span className="font-mono text-slate-800 font-bold">{log.volume_final?.toFixed(1) || "0.0"}L</span>
                            <span className="text-slate-400 font-normal ml-1">alvo {log.volume_alvo}L</span>
                          </td>
                          <td className="p-3 font-mono text-slate-600">{formatarDuracao(log.duracao_ms)}</td>
                          <td className="p-3 font-mono text-slate-400">{formatarDelayLinha(log.ts_fim_execucao, log.ts_recebido_servidor)}</td>
                          <td className="p-3 text-slate-500">{formatarData(log.ts_fim_execucao)}</td>
                          <td className="p-3 pr-5 text-right">
                            <span className={`px-2.5 py-0.5 rounded-full text-[10px] font-bold border ${
                              log.status === "SUCESSO" ? "bg-emerald-50 text-emerald-700 border-emerald-100" :
                              log.status === "CANCELADO" ? "bg-amber-50 text-amber-700 border-amber-100" :
                              "bg-red-50 text-red-700 border-red-100"
                            }`}>{log.status}</span>
                          </td>
                        </tr>
                      ))
                    )}
                  </tbody>
                </table>
              </div>
              
              {/* --- CONTROLES DE PAGINAÇÃO --- */}
              <div className="p-3 border-t border-slate-100 bg-slate-50/40 flex justify-between items-center text-[11px] text-slate-500 font-medium">
                <span>
                  Exibindo {logsFiltrados.length > 0 ? (currentPage - 1) * ITEMS_PER_PAGE + 1 : 0} a {Math.min(currentPage * ITEMS_PER_PAGE, logsFiltrados.length)} de {logsFiltrados.length} registros
                </span>
                
                <div className="flex items-center gap-2">
                  <button 
                    onClick={(e) => { e.stopPropagation(); handlePageChange(currentPage - 1); }} 
                    disabled={currentPage === 1} 
                    className="px-3 py-1.5 bg-white border border-slate-200 rounded-md text-slate-600 hover:bg-slate-50 disabled:text-slate-300 disabled:cursor-not-allowed transition-all shadow-sm">
                    Anterior
                  </button>
                  
                  <div className="flex items-center gap-1.5 px-2">
                    <span>Pág.</span>
                    <input 
                      type="text" 
                      value={pageInput}
                      onChange={(e) => setPageInput(e.target.value.replace(/[^0-9]/g, ''))}
                      onBlur={() => {
                        let p = parseInt(pageInput);
                        if (isNaN(p)) p = currentPage;
                        handlePageChange(p);
                      }}
                      onKeyDown={(e) => {
                        if (e.key === 'Enter') e.currentTarget.blur();
                      }}
                      className="w-10 py-1 text-center bg-white border border-slate-300 rounded outline-none focus:border-blue-500 focus:ring-1 focus:ring-blue-500 text-slate-700 font-bold transition-all"
                    />
                    <span>de {totalPages}</span>
                  </div>

                  <button 
                    onClick={(e) => { e.stopPropagation(); handlePageChange(currentPage + 1); }} 
                    disabled={currentPage === totalPages} 
                    className="px-3 py-1.5 bg-white border border-slate-200 rounded-md text-slate-600 hover:bg-slate-50 disabled:text-slate-300 disabled:cursor-not-allowed transition-all shadow-sm">
                    Próximo
                  </button>
                </div>
              </div>
            </div>

          </div>
        </div>
      </main>
    </div>
  );
}