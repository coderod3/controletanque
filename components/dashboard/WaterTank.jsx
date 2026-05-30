import { useTankStore } from '../../store/useTankStore';

export default function WaterTank() {
  const volume = useTankStore((state) => state.volume);
  const capacidadeMaxima = useTankStore((state) => state.capacidadeMaxima);
  const status = useTankStore((state) => state.operationStatus);

  const percentual = Math.min(Math.max((volume / capacidadeMaxima) * 100, 0), 100);

  // Define cor da água baseado no status
  const waterColor = status === 'emergency' || status === 'error' 
    ? 'bg-red-500 shadow-[0_0_20px_rgba(239,68,68,0.5)]' 
    : 'bg-blue-500 shadow-[0_0_25px_rgba(59,130,246,0.6)]';

  return (
    <div className="flex flex-col items-center justify-center p-6 bg-white rounded-lg shadow-md border border-gray-200 h-full min-h-[380px]">
      <h2 className="text-xl font-bold mb-6 text-gray-800 tracking-tight">Status do Reservatório</h2>

      {/* Container Relativo Unificado do Tanque + Cano */}
      <div className="relative pb-12 flex flex-col items-center">
        
        {/* Corpo Principal do Tanque */}
        <div className="relative w-48 h-64 bg-gray-100 border-4 border-gray-400 rounded-b-2xl overflow-hidden shadow-inner flex flex-col justify-end">
          
          {/* Linhas de Graduação de Nível */}
          <div className="absolute inset-0 flex flex-col justify-between p-2 pt-4 pointer-events-none text-[10px] font-mono font-bold text-gray-400 z-10">
            <div className="border-b border-gray-300/60 w-full flex justify-between"><span>100%</span><span>{capacidadeMaxima.toFixed(0)}L</span></div>
            <div className="border-b border-gray-300/60 w-full flex justify-between"><span>75%</span><span>{(capacidadeMaxima*0.75).toFixed(0)}L</span></div>
            <div className="border-b border-gray-300/60 w-full flex justify-between"><span>50%</span><span>{(capacidadeMaxima*0.5).toFixed(0)}L</span></div>
            <div className="border-b border-gray-300/60 w-full flex justify-between"><span>25%</span><span>{(capacidadeMaxima*0.25).toFixed(0)}L</span></div>
            <div className="w-full flex justify-between"><span>0%</span><span>0L</span></div>
          </div>

          {/* Massa de Água Fluida */}
          <div 
            className={`w-full transition-all duration-500 ease-in-out relative ${waterColor}`}
            style={{ height: `${percentual}%` }}
          >
            {/* Efeito de Ondulação Superior */}
            {percentual > 0 && percentual < 100 && (
              <div className="absolute top-0 inset-x-0 h-2 bg-white/20 animate-pulse rounded-t-full"></div>
            )}
          </div>
        </div>

        {/* --- FIX CSS: CANO DE OUTLET INTEGRADO SEM BORDAS VAZADAS --- */}
        <div className="absolute bottom-0 left-1/2 -translate-x-1/2 w-8 h-12 flex flex-col items-center pointer-events-none">
          {/* Gargalo do cano que solda na base do tanque */}
          <div className="w-8 h-8 bg-gray-400 border-x-4 border-gray-400 relative z-0">
            {/* Água passando por dentro do cano se estiver esvaziando */}
            {status === 'draining' && (
              <div className="absolute inset-x-0 bottom-0 h-full bg-blue-500 transition-all shadow-[0_0_10px_rgba(59,130,246,0.5)]"></div>
            )}
          </div>
          {/* Flange / Bocal de saída */}
          <div className="w-10 h-3 bg-gray-500 rounded-sm border-b border-gray-600 shadow-md"></div>
        </div>

      </div>

      {/* Informações Numéricas de Roda-pé */}
      <div className="mt-4 text-center font-mono bg-slate-50 border border-slate-100 px-4 py-2 rounded-xl shadow-inner">
        <p className="text-2xl font-bold text-gray-800">{volume.toFixed(1)} <span className="text-sm text-gray-500">L</span></p>
        <p className="text-[10px] text-gray-400 font-bold uppercase tracking-wider mt-0.5">Volume em Tempo Real</p>
      </div>
    </div>
  );
}