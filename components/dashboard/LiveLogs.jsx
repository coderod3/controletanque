import { useTankStore } from '../../store/useTankStore';

export default function LiveLogs() {
  // Seletor Estrito: O componente só re-renderiza quando o array de logs mudar
  const logs = useTankStore((state) => state.logs);
 
  // Captura o prefixo definido no .env.local ou na Vercel
  const topicPrefix = process.env.NEXT_PUBLIC_MQTT_TOPIC_PREFIX || 'nx_tanque_8f7d6c5b';
 
  // Formatador de horas (Padrão 24h brasileiro)
  const formatTime = (timestamp) => {
    return new Intl.DateTimeFormat('pt-BR', {
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
    }).format(new Date(timestamp));
  };

  return (
    <div className="p-6 bg-white rounded-lg shadow-md border border-gray-200 flex flex-col h-full">
      <div className="flex justify-between items-center mb-4">
        <h2 className="text-xl font-bold text-gray-800">Terminal de Auditoria</h2>
        <span className="text-xs font-semibold px-2 py-1 bg-gray-200 text-gray-600 rounded">
          MQTT / {topicPrefix}/logs
        </span>
      </div>

      {/* Janela do Terminal */}
      <div className="bg-gray-900 rounded-md p-4 flex-grow h-64 overflow-y-auto shadow-inner border border-gray-700 custom-scrollbar">
        
        {/* Estado Vazio */}
        {logs.length === 0 ? (
          <div className="flex h-full items-center justify-center text-gray-500 font-mono text-sm">
            <span className="animate-pulse">Aguardando pacotes da placa...</span>
          </div>
        ) : (
          /* Lista de Logs */
          <ul className="space-y-2 font-mono text-xs sm:text-sm">
            {logs.map((log, index) => {
              // Coloração dinâmica baseada no tipo (para futuras expansões de B.I.)
              const corTexto = 
                log.tipo === 'error' ? 'text-red-400' : 
                log.tipo === 'warning' ? 'text-yellow-400' : 
                'text-green-400';

              return (
                <li key={index} className="flex flex-col sm:flex-row sm:items-start pb-1 border-b border-gray-800 last:border-0">
                  <span className="text-gray-500 mr-3 flex-shrink-0">
                    [{formatTime(log.time)}]
                  </span>
                  <span className={`${corTexto} break-words whitespace-pre-wrap`}>
                    {log.msg}
                  </span>
                </li>
              );
            })}
          </ul>
        )}
      </div>
    </div>
  );
}