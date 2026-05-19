import { useEffect } from 'react';
import Head from 'next/head';

import { initMqtt } from '../lib/mqttService';
import { useTankStore } from '../store/useTankStore';

import WaterTank from '../components/dashboard/WaterTank';
import TelemetryCard from '../components/dashboard/TelemetryCard';
import ControlPanel from '../components/dashboard/ControlPanel';
import SyncUserPanel from '../components/dashboard/SyncUserPanel';
import LiveLogs from '../components/dashboard/LiveLogs';

const StatusIndicator = ({ status }) => {
  const getStatusDisplay = () => {
    switch (status) {
      case 'connected':
        return { color: 'bg-green-500', text: '🟢 Conectado', label: 'ONLINE' };
      case 'reconnecting':
        return { color: 'bg-yellow-500', text: '🟡 Reconectando', label: 'RECONECTANDO' };
      case 'offline':
      default:
        return { color: 'bg-red-500', text: '🔴 Desconectado', label: 'OFFLINE' };
    }
  };
  
  const display = getStatusDisplay();
  
  return (
    <div className="flex items-center gap-2 px-4 py-2 bg-gray-800 rounded-lg">
      <div className={`w-3 h-3 rounded-full ${display.color} animate-pulse`}></div>
      <span className="text-sm font-semibold text-white">{display.label}</span>
    </div>
  );
};

export default function Dashboard() {
  
  useEffect(() => {
    initMqtt();
  }, []);

  const connectionStatus = useTankStore((state) => state.connectionStatus);

  return (
    <div className="min-h-screen bg-gray-100 p-4 md:p-8">
      <Head>
        <title>
          {connectionStatus === 'connected' ? '🟢 Nexus Tank' : connectionStatus === 'reconnecting' ? '🟡 Nexus Tank' : '🔴 Nexus Tank'}
        </title>
      </Head>

      <main className="max-w-7xl mx-auto">
        <header className="mb-8 border-b border-gray-300 pb-4 flex justify-between items-end">
          <div>
            <h1 className="text-3xl font-black text-gray-900 tracking-tight">
              NEXUS <span className="text-blue-600">OPEN-LOGIC</span>
            </h1>
            <p className="text-gray-500 font-medium">Controle de Tanque Industry 4.0</p>
          </div>
          <div className="flex flex-col items-end gap-2">
            <StatusIndicator status={connectionStatus} />
            <p className="text-xs font-bold text-gray-400 uppercase">Estação de Controle .115</p>
          </div>
        </header>

        <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
          
          <div className="lg:col-span-3 space-y-6">
            <WaterTank />
          </div>

          <div className="lg:col-span-5 space-y-6">
            <TelemetryCard />
            <ControlPanel />
          </div>

          <div className="lg:col-span-4 space-y-6 flex flex-col">
            <SyncUserPanel />
            <div className="flex-grow">
              <LiveLogs />
            </div>
          </div>

        </div>
      </main>

      <footer className="max-w-7xl mx-auto mt-8 text-center text-gray-400 text-xs uppercase tracking-widest">
        Sistema Operacional Nexus &copy; 2026 | Arquitetura Multi-Core Segura
      </footer>
    </div>
  );
}