import { useTankStore } from '../../store/useTankStore';

export default function WaterTank() {
  // Seletores Estritos
  const volume = useTankStore((state) => state.volume);
  const isOnline = useTankStore((state) => state.connectionStatus) === 'connected';

  // Proteção contra null e limites físicos do CSS (0 a 100)
  const validVolume = volume !== null ? Math.min(Math.max(volume, 0), 100) : 0;

  return (
    <div className="p-6 bg-white rounded-lg shadow-md border border-gray-200 flex flex-col items-center justify-center h-full">
      <h2 className="text-xl font-bold mb-6 text-gray-800 w-full text-left">Visão Geral</h2>

      {/* Contêiner Externo do Tanque */}
      <div className="relative w-48 h-72 border-4 border-gray-400 rounded-b-2xl rounded-t-sm bg-gray-50 overflow-hidden shadow-inner flex-shrink-0">
        
        {/* Marcadores de Nível (Régua) */}
        <div className="absolute top-0 left-0 h-full w-full flex flex-col justify-between py-2 px-1 opacity-30 pointer-events-none z-10">
          {[100, 75, 50, 25, 0].map((marca) => (
            <div key={marca} className="flex items-center">
              <div className="w-4 h-[2px] bg-gray-800"></div>
              <span className="text-xs font-bold ml-1 text-gray-800">{marca}%</span>
            </div>
          ))}
        </div>

        {/* A ÁGUA 
          O segredo está no duration-1000. Ele cria a animação fluida entre um pacote MQTT e outro.
        */}
        <div
          className={`absolute bottom-0 w-full transition-all duration-1000 ease-in-out ${
            isOnline 
              ? 'bg-gradient-to-t from-blue-600 to-blue-400' 
              : 'bg-gradient-to-t from-gray-400 to-gray-300 grayscale'
          }`}
          style={{ height: `${validVolume}%` }}
        >
          {/* Reflexo / Onda na superfície da água */}
          <div className="absolute top-0 left-0 w-full h-2 bg-white opacity-20"></div>
        </div>

        {/* Overlay de Sincronização Inicial */}
        {volume === null && (
          <div className="absolute inset-0 flex items-center justify-center bg-gray-100/50 backdrop-blur-sm z-20">
            <span className="text-xs font-bold text-gray-500 uppercase tracking-widest text-center px-2">
              Aguardando<br/>Sensores
            </span>
          </div>
        )}
      </div>
    </div>
  );
}