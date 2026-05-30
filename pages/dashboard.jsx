import { useState } from "react";
import { useTankStore } from "../store/useTankStore";
import { sendCommand } from "../lib/mqttService";
import Sidebar from "../components/Sidebar";
import Head from "next/head";

/* ── Tank 3D SVG (unchanged from v1) ── */
const TANK_H   = 240;
const TANK_TOP = 52;
const TANK_BOT = TANK_TOP + TANK_H;
const CX = 115;
const RX = 82;
const RY = 22;

function Tank3D({ level, state, isOnline }) {
  const waterY  = Math.max(TANK_TOP, TANK_BOT - (level / 100) * TANK_H);
  const waterH  = TANK_BOT - waterY;
  const isActive = state !== "idle";
  const wColor  = state === "draining" ? "#f97316" : "#3b82f6";
  const wDark   = state === "draining" ? "#c2410c" : "#1d4ed8";
  const wSurface= state === "draining" ? "#fed7aa" : "#bfdbfe";

  return (
    <svg viewBox="0 0 260 370" xmlns="http://www.w3.org/2000/svg" 
      className={`w-full max-w-[280px] transition-all duration-500 ${!isOnline ? "grayscale opacity-50" : ""}`}>
      <defs>
        <linearGradient id="bodyG" x1="0%" y1="0%" x2="100%" y2="0%">
          <stop offset="0%"   stopColor="#334155" />
          <stop offset="14%"  stopColor="#64748b" />
          <stop offset="38%"  stopColor="#cbd5e1" />
          <stop offset="55%"  stopColor="#e2e8f0" />
          <stop offset="80%"  stopColor="#64748b" />
          <stop offset="100%" stopColor="#334155" />
        </linearGradient>
        <linearGradient id="liqG" x1="0%" y1="0%" x2="100%" y2="0%">
          <stop offset="0%"   stopColor={wDark} />
          <stop offset="28%"  stopColor={wColor} />
          <stop offset="60%"  stopColor={wSurface} stopOpacity="0.88" />
          <stop offset="100%" stopColor={wDark} />
        </linearGradient>
        <linearGradient id="topCapG" x1="0%" y1="0%" x2="100%" y2="0%">
          <stop offset="0%"   stopColor="#1e293b" />
          <stop offset="30%"  stopColor="#475569" />
          <stop offset="58%"  stopColor="#94a3b8" />
          <stop offset="100%" stopColor="#1e293b" />
        </linearGradient>
        <linearGradient id="botCapG" x1="0%" y1="0%" x2="100%" y2="0%">
          <stop offset="0%"   stopColor="#0f172a" />
          <stop offset="50%"  stopColor="#1e293b" />
          <stop offset="100%" stopColor="#0f172a" />
        </linearGradient>
        <clipPath id="bodyClip">
          <rect x={CX - RX} y={TANK_TOP} width={RX * 2} height={TANK_H} />
        </clipPath>
      </defs>

      {/* Mounting bands */}
      {[TANK_TOP + 65, TANK_TOP + 135, TANK_TOP + 200].map((y, i) => (
        <g key={i}>
          <rect x={CX - RX - 3} y={y - 5} width={RX * 2 + 6} height={9} rx={1.5} fill="#475569" />
          <rect x={CX - RX - 3} y={y - 5} width={RX * 2 + 6} height={9} rx={1.5} fill="none" stroke="#64748b" strokeWidth="0.5" />
        </g>
      ))}

      {/* Bottom cap */}
      <ellipse cx={CX} cy={TANK_BOT} rx={RX} ry={RY} fill="url(#botCapG)" />

      {/* Tank body */}
      <rect x={CX - RX} y={TANK_TOP} width={RX * 2} height={TANK_H} fill="url(#bodyG)" />

      {/* Seam dashes */}
      {[-35, 0, 35].map((o) => (
        <line key={o} x1={CX + o} y1={TANK_TOP} x2={CX + o} y2={TANK_BOT}
          stroke="#64748b" strokeWidth="0.5" strokeDasharray="5,9" strokeOpacity="0.45" />
      ))}

      {/* Liquid fill */}
      {level > 0 && (
        <>
          <rect x={CX - RX} y={waterY} width={RX * 2} height={waterH}
            fill="url(#liqG)" clipPath="url(#bodyClip)" />
          {level < 98 && waterY >= TANK_TOP && (
            <ellipse cx={CX} cy={waterY} rx={RX} ry={RY * 0.72} fill={wSurface} fillOpacity="0.92" />
          )}
          {level < 98 && waterY >= TANK_TOP && (
            <ellipse cx={CX - 18} cy={waterY - 2} rx={RX * 0.38} ry={RY * 0.28}
              fill="white" fillOpacity="0.22" />
          )}
        </>
      )}

      {/* Bubbles when filling */}
      {state === "filling" && level > 0 && level < 95 && (
        <>
          <circle cx={CX - 18} cy={waterY + 22} r={3.5} fill={wColor} fillOpacity="0.55" />
          <circle cx={CX + 12} cy={waterY + 38} r={2.2} fill={wColor} fillOpacity="0.45" />
          <circle cx={CX - 4}  cy={waterY + 14} r={1.6} fill="white"  fillOpacity="0.35" />
          <circle cx={CX + 28} cy={waterY + 28} r={2.8} fill={wColor} fillOpacity="0.4"  />
        </>
      )}

      {/* Body outlines */}
      <line x1={CX - RX} y1={TANK_TOP} x2={CX - RX} y2={TANK_BOT} stroke="#475569" strokeWidth="1.5" />
      <line x1={CX + RX} y1={TANK_TOP} x2={CX + RX} y2={TANK_BOT} stroke="#475569" strokeWidth="1.5" />

      {/* Specular highlight */}
      <rect x={CX + 22} y={TANK_TOP + 18} width={11} height={TANK_H - 36}
        rx={5.5} fill="white" fillOpacity="0.08" />

      {/* Top cap */}
      <ellipse cx={CX} cy={TANK_TOP} rx={RX} ry={RY} fill="url(#topCapG)" />
      <ellipse cx={CX} cy={TANK_TOP} rx={RX} ry={RY} fill="none" stroke="#64748b" strokeWidth="1" />
      <ellipse cx={CX - 18} cy={TANK_TOP - 6} rx={RX * 0.42} ry={RY * 0.46}
        fill="white" fillOpacity="0.14" />

      {/* Manhole */}
      <ellipse cx={CX} cy={TANK_TOP} rx={24} ry={7.5} fill="#1e293b" />
      <ellipse cx={CX} cy={TANK_TOP} rx={24} ry={7.5} fill="none" stroke="#475569" strokeWidth="1" />
      <ellipse cx={CX} cy={TANK_TOP} rx={15} ry={4.8} fill="none" stroke="#64748b" strokeWidth="0.5" />

      {/* Inlet pipe (top-left) */}
      <rect x={CX - RX - 26} y={TANK_TOP + 12} width={28} height={11} fill="#334155" />
      <line x1={CX - RX - 26} y1={TANK_TOP + 12} x2={CX - RX} y2={TANK_TOP + 12} stroke="#475569" strokeWidth="0.5" />
      <line x1={CX - RX - 26} y1={TANK_TOP + 23} x2={CX - RX} y2={TANK_TOP + 23} stroke="#475569" strokeWidth="0.5" />
      <rect x={CX - RX - 32} y={TANK_TOP + 8}  width={9}  height={19} rx={2.5} fill="#1e293b" />
      <text x={CX - RX - 20} y={TANK_TOP + 40} fontSize="7.5" fill="#64748b" textAnchor="middle" fontFamily="monospace">INLET</text>

      {/* Outlet pipe (bottom-right) */}
      <rect x={CX + RX - 2}  y={TANK_BOT - 15} width={32} height={11} fill="#334155" />
      <line x1={CX + RX}     y1={TANK_BOT - 15} x2={CX + RX + 30} y2={TANK_BOT - 12} stroke="#475569" strokeWidth="0.5" />
      <line x1={CX + RX}     y1={TANK_BOT - 3}  x2={CX + RX + 30} y2={TANK_BOT - 1}  stroke="#475569" strokeWidth="0.5" />
      <rect x={CX + RX + 28} y={TANK_BOT - 18} width={9}  height={19} rx={2.5} fill="#1e293b" />
      <text x={CX + RX + 32} y={TANK_BOT + 20} fontSize="7.5" fill="#64748b" textAnchor="middle" fontFamily="monospace">OUTLET</text>

      {/* Level ruler */}
      <line x1={CX + RX + 5} y1={TANK_TOP} x2={CX + RX + 5} y2={TANK_BOT} stroke="#475569" strokeWidth="0.5" />
      {[0, 25, 50, 75, 100].map((pct) => {
        const y = TANK_BOT - (pct / 100) * TANK_H;
        return (
          <g key={pct}>
            <line x1={CX + RX + 3} y1={y} x2={CX + RX + 12} y2={y} stroke="#64748b" strokeWidth="1" />
            <text x={CX + RX + 15} y={y + 3.5} fontSize="9" fill="#94a3b8" fontFamily="monospace">{pct}%</text>
          </g>
        );
      })}

      {/* Current level pointer */}
      {level > 0 && (
        <g>
          <line
            x1={CX - RX - 8} y1={waterY} x2={CX + RX + 46} y2={waterY}
            stroke={isActive ? wColor : "#f59e0b"}
            strokeWidth="1" strokeDasharray="3,3" strokeOpacity="0.85"
          />
          <polygon
            points={`${CX - RX - 8},${waterY - 5} ${CX - RX - 8},${waterY + 5} ${CX - RX - 16},${waterY}`}
            fill={isActive ? wColor : "#f59e0b"}
          />
          <text x={CX + RX + 50} y={waterY + 4} fontSize="11" fontWeight="bold"
            fill={isActive ? wColor : "#f59e0b"} fontFamily="monospace">
            {level}%
          </text>
        </g>
      )}
    </svg>
  );
}

/* ── helpers ── */
const LOG_DOT = { success: "bg-emerald-500", info: "bg-blue-500", warning: "bg-amber-400", error: "bg-red-500", comando: "bg-purple-500" };
const BOARD_STATE_META = {
  idle:     { label: "Ocioso",   cls: "text-slate-600 bg-slate-100 border-slate-200" },
  filling:  { label: "Enchendo...",  cls: "text-blue-700 bg-blue-50 border-blue-200" },
  draining: { label: "Esvaziando...",cls: "text-orange-700 bg-orange-50 border-orange-200" },
  error:    { label: "Erro",         cls: "text-red-700 bg-red-50 border-red-200" },
};

function SpinIcon({ cls = "w-4 h-4" }) {
  return (
    <svg className={`${cls} animate-spin`} fill="none" viewBox="0 0 24 24">
      <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" />
      <path className="opacity-75" fill="currentColor"
        d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z" />
    </svg>
  );
}

/* ══════════════════════════════════════════════════════════
   DASHBOARD MAIN
══════════════════════════════════════════════════════════ */
export default function Dashboard() {
  const [volume, setVolume] = useState(1.0);
  const [operation, setOperation] = useState("fill");
  const [sidebar, setSidebar] = useState(true);

  // Estados Globais
  const userRole         = useTankStore((state) => state.userRole);
  const setUserRole      = useTankStore((state) => state.setUserRole); // Usado na ferramenta de debug no topo
  const mqttOk           = useTankStore((state) => state.mqttOk);
  const boardOnline      = useTankStore((state) => state.boardOnline);
  const tankLevelLiters  = useTankStore((state) => state.volume);
  const boardState       = useTankStore((state) => state.operationStatus);
  const isSending        = useTankStore((state) => state.isSending);
  const logs             = useTankStore((state) => state.logs);
  const capacidadeMaxima = useTankStore((state) => state.capacidadeMaxima);

  // Derivações e Permissões
  const isReadOnly = userRole === "visualizacao";
  const busy = boardState !== "idle" || isSending;
  const porcentagem = Math.min(100, Math.max(0, (tankLevelLiters / capacidadeMaxima) * 100)).toFixed(0);

  return (
    <div className="min-h-screen bg-slate-50 flex overflow-hidden">
      <Head>
        <title>Painel de Controle - Nexus OS</title>
      </Head>
      {/* Mobile overlay */}
      {sidebar && <div className="fixed inset-0 bg-slate-900/60 z-20 md:hidden backdrop-blur-sm" onClick={() => setSidebar(false)} />}

      {/* Componente Sidebar Extraído */}
      <Sidebar isOpen={sidebar} activeRoute="/dashboard" />

      {/* MAIN */}
      <main className="flex-1 flex flex-col h-screen overflow-hidden">
        {/* Topbar */}
        {/* Topbar Padronizada */}
        <header className="bg-white px-5 py-4 flex items-center justify-between shadow-sm z-10 shrink-0 relative">
          <div className="flex items-center gap-3">
            <button onClick={() => setSidebar(!sidebar)} className="p-2 rounded-lg bg-slate-50 hover:bg-slate-100 border border-slate-200 transition-colors">
              <svg className="w-5 h-5 text-slate-600" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 6h16M4 12h16M4 18h16" /></svg>
            </button>
            <h1 className="font-bold tracking-tight text-lg text-slate-800 hidden sm:block">
              Sistema Operacional
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

        {/* ══ SCROLLABLE CONTENT (EXPANDIDO HORIZONTALMENTE COM w-full e px-fluid) ══ */}
        <div className="flex-1 overflow-y-auto bg-slate-50 w-full px-6 lg:px-12 py-6">
          <div className="w-full flex flex-col gap-5">

            {/* ── HEADER CARD ── */}
            <div className="bg-white rounded-xl border border-slate-200 shadow-sm px-6 py-4 flex justify-between items-center">
              <div>
                <span className="text-[10px] font-bold uppercase tracking-widest text-blue-700 bg-blue-50 border border-blue-200 px-2.5 py-1 rounded mb-2 inline-block">
                  Monitoramento em Tempo Real
                </span>
                <h1 className="text-xl font-bold text-slate-800 tracking-tight mt-1">Painel de Telemetria</h1>
                <p className="text-xs text-slate-400 mt-0.5">Controle e monitoramento do tanque industrial via MQTT.</p>
              </div>
              <div className="flex items-center gap-2 text-xs text-slate-500 font-medium shrink-0">
                <span className="w-1.5 h-1.5 rounded-full bg-emerald-500 inline-block" />
                Última leitura: {logs[0]?.time ?? "—"}
              </div>
            </div>

            {/* ── 4 STATUS CARDS ── */}
            <div className="grid grid-cols-4 gap-3">
              <div className="bg-white rounded-xl border border-slate-200 shadow-sm p-4">
                <div className="flex justify-between items-center mb-2.5">
                  <span className="text-[10px] font-bold uppercase tracking-widest text-slate-400">Placa IoT</span>
                  <span className={`w-2.5 h-2.5 rounded-full ${boardOnline ? "bg-emerald-500" : "bg-red-500"}`} />
                </div>
                <p className={`text-base font-bold mb-0.5 ${boardOnline ? "text-emerald-700" : "text-red-700"}`}>{boardOnline ? "Online" : "Offline"}</p>
                <p className="text-[10px] text-slate-400 font-mono mb-2.5">ESP32-S3 · 192.168.1.45</p>
              </div>

              <div className="bg-white rounded-xl border border-slate-200 shadow-sm p-4">
                <div className="flex justify-between items-center mb-2.5">
                  <span className="text-[10px] font-bold uppercase tracking-widest text-slate-400">MQTT Broker</span>
                  <span className={`w-2.5 h-2.5 rounded-full ${mqttOk ? "bg-emerald-500" : "bg-amber-400"}`} />
                </div>
                <p className={`text-base font-bold mb-0.5 ${mqttOk ? "text-emerald-700" : "text-amber-700"}`}>{mqttOk ? "Conectado" : "Desconect."}</p>
                <p className="text-[10px] text-slate-400 font-mono mb-2.5 truncate">broker.hivemq.com :1883</p>
              </div>

              <div className="bg-white rounded-xl border border-slate-200 shadow-sm p-4">
                <div className="flex justify-between items-center mb-2.5">
                  <span className="text-[10px] font-bold uppercase tracking-widest text-slate-400">Estado Atual</span>
                  <svg className="w-3.5 h-3.5 text-slate-400" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M13 10V3L4 14h7v7l9-11h-7z" /></svg>
                </div>
                <div className="flex items-center gap-2 mb-2">
                  {boardOnline && busy && <SpinIcon cls="w-3.5 h-3.5 text-blue-500" />}
                  <span className={`text-xs font-bold px-2.5 py-1 rounded-md border ${!boardOnline ? "text-slate-400 bg-slate-50 border-slate-200" : (BOARD_STATE_META[boardState]?.cls || BOARD_STATE_META['idle'].cls)}`}>
                    {!boardOnline ? "Desconectado" : (BOARD_STATE_META[boardState]?.label || "Aguardando")}
                  </span>
                </div>
                <p className="text-[10px] text-slate-400">Último evento: <span className="font-mono">{logs[0]?.time ?? "—"}</span></p>
              </div>

              <div className="bg-white rounded-xl border border-slate-200 shadow-sm p-4">
                <div className="flex justify-between items-center mb-2.5">
                  <span className="text-[10px] font-bold uppercase tracking-widest text-slate-400">Nível do Tanque</span>
                </div>
                <p className={`text-2xl font-bold font-mono mb-1.5 ${boardOnline ? "text-blue-700" : "text-slate-300"}`}>{boardOnline ? `${porcentagem}%` : "---"}</p>
                <div className="w-full h-1.5 bg-slate-100 rounded-full overflow-hidden mb-1.5">
                  <div className={`h-full rounded-full transition-all duration-700 ${!boardOnline ? "bg-slate-200" : porcentagem > 80 ? "bg-blue-600" : porcentagem > 30 ? "bg-blue-500" : "bg-amber-500"}`} style={{ width: `${boardOnline ? porcentagem : 0}%` }} />
                </div>
                <p className="text-[10px] text-slate-400">{boardOnline ? `${tankLevelLiters.toFixed(1)} L / ${capacidadeMaxima.toFixed(1)} L` : "Sem conexão"}</p>
              </div>
            </div>

            {/* ── TANK (2fr) + COMMAND PANEL (1fr) ── */}
            <div className="grid grid-cols-3 gap-4">

              {/* Tank visualization */}
              <div className="col-span-2 bg-white rounded-xl border border-slate-200 shadow-sm overflow-hidden">
                <div className="px-5 py-3.5 border-b border-slate-100 bg-slate-50/60 flex justify-between items-center">
                  <div>
                    <p className="font-bold text-slate-800 text-sm">Visualização 3D do Tanque</p>
                    <p className="text-[11px] text-slate-400 mt-0.5">Representação em tempo real do nível de fluido</p>
                  </div>
                  <div className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg border text-xs font-bold shrink-0 ${!boardOnline ? "bg-slate-50 text-slate-400 border-slate-200" : boardState === "filling"  ? "bg-blue-50 text-blue-700 border-blue-200" : boardState === "draining" ? "bg-orange-50 text-orange-700 border-orange-200" : "bg-slate-50 text-slate-500 border-slate-200"}`}>
                    {boardOnline && busy && <SpinIcon cls="w-3 h-3" />}
                    {!boardOnline ? "Desconectado" : boardState === "filling" ? "Enchendo" : boardState === "draining" ? "Esvaziando" : "Ocioso"}
                  </div>
                </div>
                <div className="flex justify-center items-center py-6 bg-white">
                  <Tank3D level={boardOnline ? porcentagem : 0} state={boardOnline ? boardState : "idle"} isOnline={boardOnline} />
                </div>
                <div className="px-5 py-3.5 border-t border-slate-100">
                  <div className="flex justify-between text-xs mb-1.5">
                    <span className="text-slate-500">Nível atual</span>
                    <span className={`font-bold ${boardOnline ? "text-slate-700" : "text-slate-300"}`}>{boardOnline ? `${porcentagem}% · ${tankLevelLiters.toFixed(1)} L / ${capacidadeMaxima.toFixed(1)} L` : "--- · Sem conexão"}</span>
                  </div>
                  <div className="w-full h-3 bg-slate-100 rounded-full overflow-hidden">
                    <div className={`h-full rounded-full transition-all duration-700 ${!boardOnline ? "bg-slate-200" : porcentagem > 80 ? "bg-blue-600" : porcentagem > 30 ? "bg-blue-500" : "bg-amber-500"}`} style={{ width: `${boardOnline ? porcentagem : 0}%` }} />
                  </div>
                  <div className="flex justify-between text-[10px] text-slate-400 mt-1.5">
                    <span>0%</span>
                    {boardOnline && porcentagem < 20 && <span className="text-amber-500 font-bold">⚠ Nível crítico</span>}
                    {boardOnline && porcentagem > 90 && <span className="text-blue-500 font-bold">Tanque quase cheio</span>}
                    <span>100%</span>
                  </div>
                </div>
              </div>

              {/* ── COMMAND PANEL (COM BLOQUEIO DE CARGO) ── */}
              <div className="col-span-1 bg-white rounded-xl border border-slate-200 shadow-sm overflow-hidden flex flex-col relative">
                
                {/* Overlay de Bloqueio Visual se for apenas visualização */}
                {isReadOnly && (
                  <div className="absolute inset-0 bg-slate-50/60 backdrop-blur-[1px] z-10 flex flex-col items-center justify-center p-6 text-center border border-amber-100/50">
                    <div className="bg-amber-100 text-amber-700 p-3 rounded-full mb-3 shadow-sm border border-amber-200">
                      <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M12 15v2m-6 4h12a2 2 0 002-2v-6a2 2 0 00-2-2H6a2 2 0 00-2 2v6a2 2 0 002 2zm10-10V7a4 4 0 00-8 0v4h8V7z" /></svg>
                    </div>
                    <span className="font-bold text-slate-800 text-sm mb-1">Acesso Restrito</span>
                    <p className="text-[11px] text-slate-500 font-medium">O seu cargo de visualização não permite acionar bombas ou válvulas operacionais.</p>
                  </div>
                )}

                <div className="px-5 py-3.5 border-b border-slate-100 bg-slate-50/60">
                  <p className="font-bold text-slate-800 text-sm">Enviar Comando</p>
                  <p className="text-[11px] text-slate-400 mt-0.5">Controle manual do tanque</p>
                </div>

                <div className="flex-1 flex flex-col gap-4 p-5">
                  <div>
                    <span className="block text-[10px] font-bold uppercase tracking-widest text-slate-500 mb-2">Operação</span>
                    <div className="grid grid-cols-2 gap-2">
                      <button onClick={() => setOperation("fill")} disabled={isReadOnly}
                        className={`py-2.5 rounded-lg text-sm font-bold border transition-colors ${operation === "fill" ? "bg-blue-600 text-white border-blue-600 shadow-sm shadow-blue-600/20" : "bg-slate-50 text-slate-500 border-slate-200 hover:border-blue-300 hover:text-blue-600"} disabled:opacity-50 disabled:pointer-events-none`}>
                        ↑ Encher
                      </button>
                      <button onClick={() => setOperation("drain")} disabled={isReadOnly}
                        className={`py-2.5 rounded-lg text-sm font-bold border transition-colors ${operation === "drain" ? "bg-orange-500 text-white border-orange-500 shadow-sm shadow-orange-500/20" : "bg-slate-50 text-slate-500 border-slate-200 hover:border-orange-300 hover:text-orange-500"} disabled:opacity-50 disabled:pointer-events-none`}>
                        ↓ Esvaziar
                      </button>
                    </div>
                  </div>

                  {(() => {
                    const limiteEncher = Math.max(0, capacidadeMaxima - tankLevelLiters);
                    const limiteEsvaziar = Math.max(0, tankLevelLiters);
                    const limiteAtual = operation === "fill" ? limiteEncher : limiteEsvaziar;
                    const maxSlider = limiteAtual < 0.1 ? 0.1 : limiteAtual; 
                    const volumeSeguro = Math.min(volume, maxSlider);
                    const isOpDisabled = limiteAtual < 0.1 || isReadOnly;

                    return (
                      <>
                        <div>
                          <div className="flex justify-between items-center mb-2">
                            <span className="text-[10px] font-bold uppercase tracking-widest text-slate-500">Volume (Litros)</span>
                            <span className="text-[10px] text-slate-400 font-mono">máx {limiteAtual.toFixed(1)} L</span>
                          </div>
                          <div className="relative mb-2">
                            <div className="w-full border border-slate-200 rounded-lg px-3 py-2.5 bg-slate-50 text-right font-mono text-sm font-bold text-slate-800 pr-8">
                              {volumeSeguro.toFixed(1)}
                            </div>
                            <span className="absolute right-3 top-1/2 -translate-y-1/2 text-xs text-slate-400 font-semibold">L</span>
                          </div>
                          <input type="range" min="0.1" max={maxSlider} step="0.1"
                            value={volumeSeguro}
                            onChange={(e) => setVolume(parseFloat(e.target.value))}
                            disabled={isOpDisabled}
                            className={`w-full ${isOpDisabled ? "cursor-not-allowed opacity-50" : "cursor-pointer"} ${operation === "fill" ? "accent-blue-600" : "accent-orange-500"}`} />
                        </div>

                        <button 
                          onClick={() => {
                            const acao = operation === "fill" ? "ENCHER" : "ESVAZIAR";
                            sendCommand(acao, volumeSeguro);
                          }}
                          disabled={busy || !boardOnline || !mqttOk || isOpDisabled || isReadOnly}
                          className={`w-full py-3 font-bold text-sm rounded-xl text-white transition-all mt-4 flex items-center justify-center gap-2
                            disabled:bg-slate-200 disabled:text-slate-400 disabled:cursor-not-allowed disabled:shadow-none
                            ${operation === "fill" ? "bg-blue-600 hover:bg-blue-700 active:bg-blue-800" : "bg-orange-500 hover:bg-orange-600 active:bg-orange-700"}`}>
                          {busy ? <><SpinIcon />{boardState === "filling" ? "Enchendo..." : "Esvaziando..."}</> : <>Enviar Comando</>}
                        </button>
                      </>
                    );
                  })()}
                </div>
              </div>
            </div>

            {/* ── ACTIVITY LOG ── */}
            <div className="bg-white rounded-xl border border-slate-200 shadow-sm overflow-hidden">
              <div className="flex items-center justify-between px-5 py-3.5 border-b border-slate-100 bg-slate-50/60">
                <div>
                  <h2 className="text-sm font-bold text-slate-800">Log de Atividades</h2>
                  <p className="text-[11px] text-slate-400 mt-0.5">Eventos em tempo real — placa, MQTT e sistema</p>
                </div>
                <div className="flex items-center gap-3">
                  <span className="text-[10px] font-bold text-slate-400 bg-slate-100 border border-slate-200 px-2.5 py-1 rounded">
                    {logs.length} eventos
                  </span>
                </div>
              </div>

              <div className="divide-y divide-slate-50 max-h-64 overflow-y-auto">
                {logs.map((log, idx) => (
                  <div key={log.id} className={`flex items-start gap-3.5 px-5 py-3 transition-colors hover:bg-slate-50/80 ${idx === 0 ? "bg-slate-50/40" : ""}`}>
                    <span className={`w-2 h-2 rounded-full mt-1.5 shrink-0 ${LOG_DOT[log.tipo]}`} />
                    <p className="flex-1 text-xs text-slate-700 leading-relaxed">{log.msg}</p>
                    <span className="text-[10px] font-mono text-slate-400 shrink-0 mt-0.5 bg-slate-100 px-1.5 py-0.5 rounded">{log.time}</span>
                  </div>
                ))}
              </div>
            </div>

          </div>
        </div>
      </main>
    </div>
  );
}