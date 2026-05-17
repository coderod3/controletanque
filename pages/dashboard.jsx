import { useEffect } from 'react';
import Head from 'next/head';

// Importação do Cérebro (Voltando apenas 1 nível: da pasta pages para a raiz)
import { initMqtt } from '../lib/mqttService';
import { useTankStore } from '../store/useTankStore';

// Importação dos Membros (Voltando 1 nível e entrando em components)
import WaterTank from '../components/dashboard/WaterTank';
import TelemetryCard from '../components/dashboard/TelemetryCard';
import ControlPanel from '../components/dashboard/ControlPanel';
import SyncUserPanel from '../components/dashboard/SyncUserPanel';
import LiveLogs from '../components/dashboard/LiveLogs';

export default function Dashboard() {
  
  // 1. Inicialização do Maestro
  useEffect(() => {
    // Dispara a conexão MQTT apenas no lado do cliente
    initMqtt();
  }, []);

  // Pegamos apenas o status da conexão para o título dinâmico na aba do navegador
  const connectionStatus = useTankStore((state) => state.connectionStatus);

  return (
    <div className="min-h-screen bg-gray-100 p-4 md:p-8">
      <Head>
        <title>
          {connectionStatus === 'connected' ? '🟢 Nexus Tank' : '🔴 Desconectado'}
        </title>
      </Head>

      <main className="max-w-7xl mx-auto">
        {/* Título Principal */}
        <header className="mb-8 border-b border-gray-300 pb-4 flex justify-between items-end">
          <div>
            <h1 className="text-3xl font-black text-gray-900 tracking-tight">
              NEXUS <span className="text-blue-600">OPEN-LOGIC</span>
            </h1>
            <p className="text-gray-500 font-medium">Controle de Tanque Industry 4.0</p>
          </div>
          <div className="text-right hidden md:block">
            <p className="text-xs font-bold text-gray-400 uppercase">Estação de Controle .115</p>
          </div>
        </header>

        {/* Dashboard Grid Layout */}
        <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
          
          {/* COLUNA 1: Visualização do Tanque (Ocupa 3 colunas) */}
          <div className="lg:col-span-3 space-y-6">
            <WaterTank />
          </div>

          {/* COLUNA 2: Telemetria e Controle (Ocupa 5 colunas) */}
          <div className="lg:col-span-5 space-y-6">
            <TelemetryCard />
            <ControlPanel />
          </div>

          {/* COLUNA 3: Gestão e Auditoria (Ocupa 4 colunas) */}
          <div className="lg:col-span-4 space-y-6 flex flex-col">
            <SyncUserPanel />
            <div className="flex-grow">
              <LiveLogs />
            </div>
          </div>

        </div>
      </main>

      {/* Rodapé de Segurança */}
      <footer className="max-w-7xl mx-auto mt-8 text-center text-gray-400 text-xs uppercase tracking-widest">
        Sistema Operacional Nexus &copy; 2026 | Arquitetura Multi-Core Segura
      </footer>
    </div>
  );
}