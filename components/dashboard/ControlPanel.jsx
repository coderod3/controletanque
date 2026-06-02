import { useState, useEffect } from 'react';
import { useTankStore } from '../../store/useTankStore';
import { sendCommand } from '../../lib/mqttService';

export default function ControlPanel() {
  const volumeAtual = useTankStore((state) => state.volume);
  const capMaxima = useTankStore((state) => state.capacidadeMaxima);
  const isSending = useTankStore((state) => state.isSending);
  const commandFeedback = useTankStore((state) => state.commandFeedback);
  const setCommandFeedback = useTankStore((state) => state.setCommandFeedback);
  const isOnline = useTankStore((state) => state.connectionStatus) === 'connected' || useTankStore((state) => state.boardOnline);
  const operationStatus = useTankStore((state) => state.operationStatus);

  // Estado local para o Slider (Bolinha)
  const [alvoInput, setAlvoInput] = useState(0);
  const [isDragging, setIsDragging] = useState(false);

  // Sincroniza a bolinha com o volume atual apenas quando ninguém está arrastando e o sistema está livre
  useEffect(() => {
    if (!isDragging && operationStatus === 'idle') {
      setAlvoInput(volumeAtual);
    }
  }, [volumeAtual, isDragging, operationStatus]);

  // ==========================================
  // CÁLCULO DE LIMITES (Site e Máquina)
  // ==========================================
  const authUser = typeof window !== 'undefined' ? JSON.parse(localStorage.getItem('authUser') || '{}') : {};
  const role = typeof window !== 'undefined' ? localStorage.getItem('userRole') || authUser.setor : '';
  const isGestor = role === 'gestor' || role === 'Gestão';

  // Cotas do Usuário
  const limiteUserEncher = isGestor ? capMaxima : (Number(authUser.limite_encher) || 0);
  const limiteUserEsvaziar = isGestor ? capMaxima : (Number(authUser.limite_esvaziar) || 0);

  // Limites Físicos Restantes no Tanque
  const maxFisicoEncher = Math.max(0, capMaxima - volumeAtual);
  const maxFisicoEsvaziar = Math.max(0, volumeAtual);

  // A Restrição Real é o Menor valor entre o Tanque e a Cota do Crachá
  const maxPermitidoEncher = Math.min(maxFisicoEncher, limiteUserEncher);
  const maxPermitidoEsvaziar = Math.min(maxFisicoEsvaziar, limiteUserEsvaziar);

  // Range de Deslizamento do Input Range
  const sliderMin = volumeAtual - maxPermitidoEsvaziar;
  const sliderMax = volumeAtual + maxPermitidoEncher;

  // Calculo de Deltas (Quantidade escolhida)
  const delta = alvoInput - volumeAtual;
  const deltaAbsoluto = Math.abs(delta);

  // ==========================================
  // HANDLERS
  // ==========================================
  const handleAcao = () => {
    if (!isOnline) {
      setCommandFeedback('Erro: A placa está Offline.');
      return;
    }
    if (delta === 0) return;

    const acao = delta > 0 ? 'ENCHER' : 'ESVAZIAR';

    setCommandFeedback('Aguardando confirmação da máquina...');
    // ATENÇÃO: O ESP32 espera a quantidade que vai subir/descer (Delta), não o alvo final absoluto!
    sendCommand(acao, deltaAbsoluto, authUser.matricula, authUser.nome);
  };

  const handleCancelar = () => {
    if (!isOnline) return;
    sendCommand('PARAR', 0, authUser.matricula, authUser.nome);
    setCommandFeedback('Comando de Cancelamento enviado!');
  };

  // Status humanizado
  const isOcupado = operationStatus !== 'idle';

  return (
    <div className="p-6 bg-white rounded-lg shadow-md border border-gray-200">
      <div className="flex justify-between items-center mb-4">
        <h2 className="text-xl font-bold text-gray-800">Painel de Operação</h2>
        {isGestor && <span className="bg-purple-100 text-purple-800 text-xs font-semibold px-2.5 py-0.5 rounded">Acesso Gestor (Sem Limites)</span>}
      </div>

      {/* TABELA DE LIMITES E INFORMAÇÕES */}
      <div className="grid grid-cols-2 gap-4 mb-6 text-sm">
        <div className="bg-blue-50 p-3 rounded border border-blue-100">
          <p className="text-blue-800 font-semibold mb-1">Permitido Encher</p>
          <p className="text-gray-600">Sua Cota: {isGestor ? 'Ilimitado' : `${limiteUserEncher} L`}</p>
          <p className="text-gray-600">Espaço Físico: {maxFisicoEncher.toFixed(1)} L</p>
          <p className="text-blue-600 font-bold mt-1">Máx Atual: {maxPermitidoEncher.toFixed(1)} L</p>
        </div>
        <div className="bg-orange-50 p-3 rounded border border-orange-100">
          <p className="text-orange-800 font-semibold mb-1">Permitido Esvaziar</p>
          <p className="text-gray-600">Sua Cota: {isGestor ? 'Ilimitado' : `${limiteUserEsvaziar} L`}</p>
          <p className="text-gray-600">Líquido Disp.: {maxFisicoEsvaziar.toFixed(1)} L</p>
          <p className="text-orange-600 font-bold mt-1">Máx Atual: {maxPermitidoEsvaziar.toFixed(1)} L</p>
        </div>
      </div>

      {/* SLIDER E AÇÕES MÚTUAMENTE EXCLUSIVOS COM O MODO "OCUPADO" */}
      {isOcupado ? (
        <div className="bg-red-50 border-2 border-red-200 rounded-lg p-6 text-center animate-pulse">
          <h3 className="text-red-800 font-bold text-lg mb-2">Máquina Ocupada / Em Operação</h3>
          <p className="text-red-600 mb-4 text-sm">Um processo está em andamento (Painel Físico ou Web).</p>
          <button
            onClick={handleCancelar}
            disabled={isSending || !isOnline}
            className="w-full bg-red-600 hover:bg-red-700 text-white font-bold py-4 px-4 rounded transition-colors shadow-sm disabled:opacity-50"
          >
            🛑 CANCELAR OPERAÇÃO AGORA
          </button>
        </div>
      ) : (
        <>
          {/* SELETOR SLIDER (Bolinha Dinâmica) */}
          <div className="mb-6 bg-gray-50 p-4 rounded-lg border border-gray-200">
            <div className="flex justify-between items-end mb-2">
              <label className="text-sm font-medium text-gray-700">Ajuste o Nível Alvo:</label>
              <span className="text-2xl font-bold text-gray-800">{alvoInput.toFixed(1)} <span className="text-base font-normal text-gray-500">L</span></span>
            </div>

            <input
              type="range"
              min={sliderMin}
              max={sliderMax}
              step="0.5"
              value={alvoInput}
              onPointerDown={() => setIsDragging(true)}
              onPointerUp={() => setIsDragging(false)}
              onChange={(e) => setAlvoInput(Number(e.target.value))}
              disabled={isSending || !isOnline}
              className="w-full h-3 bg-gray-300 rounded-lg appearance-none cursor-pointer accent-blue-600 disabled:opacity-50"
            />
            
            <div className="flex justify-between text-xs text-gray-500 mt-2 font-medium">
              <span>{sliderMin.toFixed(0)}L (Mínimo Permitido)</span>
              <span>{volumeAtual.toFixed(1)}L (Atual)</span>
              <span>{sliderMax.toFixed(0)}L (Máximo Permitido)</span>
            </div>
          </div>

          {/* INDICADOR DE AÇÃO */}
          <div className="mb-6">
            {delta > 0 && (
              <div className="text-center p-3 bg-blue-100 text-blue-800 rounded-md font-semibold">
                Você pedirá para ENCHER +{deltaAbsoluto.toFixed(1)} Litros
              </div>
            )}
            {delta < 0 && (
              <div className="text-center p-3 bg-orange-100 text-orange-800 rounded-md font-semibold">
                Você pedirá para ESVAZIAR -{deltaAbsoluto.toFixed(1)} Litros
              </div>
            )}
            {delta === 0 && (
              <div className="text-center p-3 bg-gray-100 text-gray-500 rounded-md font-medium">
                Deslize a barra para definir uma nova tarefa.
              </div>
            )}
          </div>

          {/* BOTÃO DE ENVIAR */}
          <button
            onClick={handleAcao}
            disabled={isSending || !isOnline || delta === 0}
            className={`w-full text-white font-bold py-4 px-4 rounded shadow-md transition-all 
              ${delta > 0 ? 'bg-blue-600 hover:bg-blue-700' : 
                delta < 0 ? 'bg-orange-500 hover:bg-orange-600' : 
                'bg-gray-400 cursor-not-allowed'}`}
          >
            {delta > 0 ? '💧 INICIAR ENCHIMENTO' : delta < 0 ? '🔥 INICIAR ESVAZIAMENTO' : 'AGUARDANDO...'}
          </button>
        </>
      )}

      {/* Feedback do Sistema */}
      {commandFeedback && (
        <div className={`mt-4 p-3 rounded text-sm font-medium text-center shadow-sm ${
          commandFeedback.includes('Erro') || commandFeedback.includes('Timeout') || commandFeedback.includes('Negado')
            ? 'bg-red-100 text-red-700 border border-red-200'
            : commandFeedback.includes('Aguardando')
            ? 'bg-yellow-100 text-yellow-700 animate-pulse border border-yellow-200'
            : 'bg-green-100 text-green-700 border border-green-200'
        }`}>
          {commandFeedback}
        </div>
      )}
    </div>
  );
}