import { useState } from 'react';
import { useTankStore } from '../../store/useTankStore';
import { sendCommand } from '../../lib/mqttService'; // (Só onde tiver o sendCommand)

export default function SyncUserPanel() {
  // 1. Estado Local (Apenas para o formulário)
  const [uid, setUid] = useState('');
  const [nome, setNome] = useState('');

  // 2. Estado Global (Seletor Estrito para performance)
  const isSending = useTankStore((state) => state.isSending);
  const isOnline = useTankStore((state) => state.connectionStatus) === 'connected';

  // 3. Submissão do Formulário
  const handleSync = (e) => {
    e.preventDefault(); // Evita que a página recarregue

    if (!isOnline || isSending) return;

    // Formata o UID: remove espaços e força maiúsculas para bater exato com o C++
    const uidFormatado = uid.trim().toUpperCase();
    const nomeFormatado = nome.trim();

    if (uidFormatado && nomeFormatado) {
      // Dispara via MQTT. O valor da bomba vai como -1 (ignorado pela placa neste comando)
      sendCommand('SYNC_USER', -1, uidFormatado, nomeFormatado);
      
      // Limpa os campos após o envio para facilitar novo cadastro
      setUid('');
      setNome('');
    }
  };

  return (
    <div className="p-6 bg-white rounded-lg shadow-md border border-gray-200">
      <h2 className="text-xl font-bold mb-4 text-gray-800">Sincronizar Operador</h2>
      <p className="text-sm text-gray-500 mb-6">
        Cadastre temporariamente um cartão RFID na memória física da máquina.
      </p>

      <form onSubmit={handleSync}>
        {/* Campo UID */}
        <div className="mb-4">
          <label className="block text-sm font-medium text-gray-700 mb-1">
            UID do Cartão (Hexadecimal)
          </label>
          <input
            type="text"
            required
            placeholder="Ex: A1B2C3D4"
            value={uid}
            onChange={(e) => setUid(e.target.value)}
            disabled={isSending || !isOnline}
            className="w-full px-3 py-2 border rounded-md uppercase focus:ring-2 focus:ring-blue-500 outline-none disabled:bg-gray-100 disabled:text-gray-400"
          />
        </div>

        {/* Campo Nome */}
        <div className="mb-6">
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Nome do Operador
          </label>
          <input
            type="text"
            required
            placeholder="Ex: Engenheiro Rodrigo"
            value={nome}
            onChange={(e) => setNome(e.target.value)}
            disabled={isSending || !isOnline}
            className="w-full px-3 py-2 border rounded-md focus:ring-2 focus:ring-blue-500 outline-none disabled:bg-gray-100 disabled:text-gray-400"
          />
        </div>

        {/* Botão de Envio */}
        <button
          type="submit"
          disabled={isSending || !isOnline || !uid || !nome}
          className="w-full bg-indigo-600 hover:bg-indigo-700 text-white font-bold py-2 px-4 rounded transition-colors disabled:opacity-50 disabled:cursor-not-allowed flex justify-center items-center"
        >
          {/* Altera o texto se estiver processando */}
          {isSending ? (
            <span className="animate-pulse">Sincronizando...</span>
          ) : (
            'Enviar para a Placa'
          )}
        </button>
      </form>
    </div>
  );
}