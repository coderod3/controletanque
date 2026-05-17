import { useTankStore } from '../../store/useTankStore';

export default function TelemetryCard() {
  // Seletores estritos do Zustand (Garante performance)
  const volume = useTankStore((state) => state.volume);
  const operationStatus = useTankStore((state) => state.operationStatus);
  const connectionStatus = useTankStore((state) => state.connectionStatus);

  // Mapeamento visual para a saúde da conexão
  const connectionConfig = {
    connected: { color: 'bg-green-500', text: 'Online', textColor: 'text-green-700', bg: 'bg-green-100' },
    reconnecting: { color: 'bg-yellow-500 animate-pulse', text: 'Reconectando', textColor: 'text-yellow-700', bg: 'bg-yellow-100' },
    offline: { color: 'bg-red-500', text: 'Offline', textColor: 'text-red-700', bg: 'bg-red-100' },
  };

  const statusVis = connectionConfig[connectionStatus] || connectionConfig.offline;

  return (
    <div className="p-6 bg-white rounded-lg shadow-md border border-gray-200 flex flex-col justify-between">
      
      {/* Cabeçalho: Título e Status de Conexão */}
      <div className="flex justify-between items-center mb-6">
        <h2 className="text-xl font-bold text-gray-800">Telemetria ao Vivo</h2>
        
        {/* Badge de Conexão */}
        <div className={`flex items-center px-3 py-1 rounded-full ${statusVis.bg} ${statusVis.textColor} text-sm font-semibold`}>
          <span className={`w-2.5 h-2.5 rounded-full mr-2 ${statusVis.color}`}></span>
          {statusVis.text}
        </div>
      </div>

      {/* Corpo: O Nível do Tanque */}
      <div className="text-center my-4">
        <p className="text-sm font-medium text-gray-500 uppercase tracking-wide mb-1">
          Nível Atual
        </p>
        <div className="text-6xl font-black text-blue-600">
          {/* Se for null, mostra traços. Caso contrário, formata com 1 casa decimal */}
          {volume !== null ? `${Number(volume).toFixed(1)}%` : '--.-%'}
        </div>
      </div>

      {/* Rodapé: Estado da Máquina (FSM) */}
      <div className="mt-6 pt-4 border-t border-gray-100">
        <div className="flex justify-between items-center">
          <span className="text-gray-500 text-sm">Estado Operacional:</span>
          <span className={`font-bold px-2 py-1 rounded text-sm ${
            operationStatus === 'EXECUTANDO' ? 'bg-blue-100 text-blue-700' :
            operationStatus === 'ESPERANDO_RFID' ? 'bg-purple-100 text-purple-700' :
            'bg-gray-100 text-gray-700'
          }`}>
            {volume === null ? 'AGUARDANDO DADOS' : operationStatus}
          </span>
        </div>
      </div>
      
    </div>
  );
}