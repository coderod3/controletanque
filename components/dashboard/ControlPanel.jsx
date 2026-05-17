import { useState } from 'react';
import { useTankStore } from '../../store/useTankStore';
import { sendCommand } from '../../lib/mqttService'; // (Só onde tiver o sendCommand)

export default function ControlPanel() {
  // 1. Estado Local: Apenas para o que o usuário está digitando agora
  const [alvoInput, setAlvoInput] = useState(50);

  // 2. Estado Global (Seletor Estrito): Otimiza a performance ignorando mudanças de "volume"
  const isSending = useTankStore((state) => state.isSending);
  const commandFeedback = useTankStore((state) => state.commandFeedback);
  const isOnline = useTankStore((state) => state.connectionStatus) === 'connected';

  // 3. Handlers de Controle
  const handleAcao = (acao) => {
    // Validação de segurança dupla
    if (!isOnline) return;
    
    let valorAlvo = -1;
    if (acao === 'ENCHER' || acao === 'ESVAZIAR') {
      valorAlvo = Number(alvoInput);
      if (valorAlvo < 0) valorAlvo = 0;
      if (valorAlvo > 100) valorAlvo = 100;
    }

    // Dispara o comando (Fire and Forget - a Store cuida do resto)
    sendCommand(acao, valorAlvo);
  };

  return (
    <div className="p-6 bg-white rounded-lg shadow-md border border-gray-200">
      <h2 className="text-xl font-bold mb-4 text-gray-800">Painel de Operação</h2>

      {/* Input de Volume Alvo */}
      <div className="mb-6">
        <label className="block text-sm font-medium text-gray-700 mb-2">
          Volume Alvo (%)
        </label>
        <div className="flex items-center space-x-3">
          <input
            type="number"
            min="0"
            max="100"
            step="5"
            value={alvoInput}
            onChange={(e) => setAlvoInput(e.target.value)}
            disabled={isSending || !isOnline}
            className="w-24 px-3 py-2 border rounded-md text-lg text-center disabled:bg-gray-100 disabled:text-gray-400"
          />
          <span className="text-gray-500">%</span>
        </div>
      </div>

      {/* Botões de Ação */}
      <div className="grid grid-cols-2 gap-3 mb-4">
        <button
          onClick={() => handleAcao('ENCHER')}
          disabled={isSending || !isOnline}
          className="bg-blue-600 hover:bg-blue-700 text-white font-bold py-3 px-4 rounded transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
        >
          Encher
        </button>
        
        <button
          onClick={() => handleAcao('ESVAZIAR')}
          disabled={isSending || !isOnline}
          className="bg-orange-500 hover:bg-orange-600 text-white font-bold py-3 px-4 rounded transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
        >
          Esvaziar
        </button>
      </div>

      {/* Emergência - Ocupa a largura toda */}
      <button
        onClick={() => handleAcao('PARAR')}
        disabled={isSending || !isOnline}
        className="w-full bg-red-600 hover:bg-red-700 text-white font-bold py-3 px-4 rounded transition-colors shadow-sm disabled:opacity-50 disabled:cursor-not-allowed"
      >
        🛑 PARAR TUDO
      </button>

      {/* Feedback do Sistema (Timeout ou Confirmação) */}
      {commandFeedback && (
        <div className={`mt-4 p-3 rounded text-sm font-medium text-center ${
          commandFeedback.includes('Erro') || commandFeedback.includes('Timeout')
            ? 'bg-red-100 text-red-700'
            : commandFeedback.includes('Aguardando')
            ? 'bg-yellow-100 text-yellow-700 animate-pulse'
            : 'bg-green-100 text-green-700'
        }`}>
          {commandFeedback}
        </div>
      )}
    </div>
  );
}