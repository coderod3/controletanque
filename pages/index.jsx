import { useState } from 'react';
import Head from 'next/head';
import { useRouter } from 'next/router';
import { useTankStore } from '../store/useTankStore';

export default function Login() {
  const router = useRouter();
  const [matricula, setMatricula] = useState('');
  const [password, setPassword] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [errorMsg, setErrorMsg] = useState('');
  
  const [bgColor, setBgColor] = useState('#0284c7');

  const handleLogin = async (e) => {
    e.preventDefault();
    setIsLoading(true);
    setErrorMsg('');

    try {
      const res = await fetch('/api/auth', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ matricula, password })
      });

      if (res.ok) {
        const data = await res.json();
        
        // CORREÇÃO: Salvar o objeto do utilizador (incluindo as cotas) no cache do navegador
        localStorage.setItem('authUser', JSON.stringify(data.user)); 
        
        // Injeta o cargo validado no banco de dados para dentro do sistema
        useTankStore.getState().setUserRole(data.role);
        
        // Salva o cargo num cookie para o SSR
        document.cookie = `nexus_role=${data.role}; path=/; max-age=86400; SameSite=Strict`;
        
        router.push('/dashboard');
      }
      else {
        const errorData = await res.json();
        setErrorMsg(errorData.message || 'Credenciais inválidas ou sem permissão.');
        setIsLoading(false);
      }
    } catch (error) {
      setErrorMsg('Erro ao conectar com o servidor.');
      setIsLoading(false);
    }
  };

  return (
    <div 
      className="min-h-screen flex flex-col items-center justify-center p-4 relative overflow-hidden transition-colors duration-500"
      style={{ backgroundColor: bgColor }}
    >
      <Head>
        <title>Autenticação - Nexus OS</title>
      </Head>

      <div className="fixed bottom-6 left-6 bg-black/20 backdrop-blur-md p-3 rounded-xl border border-white/20 shadow-2xl z-50 flex items-center gap-3 hover:bg-black/30 transition-all">
        <label htmlFor="colorPicker" className="text-xs font-semibold text-white drop-shadow-md cursor-pointer">
          Tom do Fundo:
        </label>
        <input
          id="colorPicker"
          type="color"
          value={bgColor}
          onChange={(e) => setBgColor(e.target.value)}
          className="w-10 h-10 rounded cursor-pointer border-0 p-0 bg-transparent"
          title="Escolha a cor de fundo"
        />
        <span className="text-xs text-white/90 font-mono uppercase bg-black/20 px-2 py-1 rounded">{bgColor}</span>
      </div>

      <div className="absolute inset-0 pointer-events-none overflow-hidden z-0">
        <img 
          src="/img/rice.png" 
          alt="" 
          className="absolute -left-10 bottom-10 h-[75vh] w-auto object-contain opacity-30 brightness-0 invert drop-shadow-[0_4px_10px_rgba(255,255,255,0.1)]"
        />
        <img 
          src="/img/corn.png" 
          alt="" 
          className="absolute -right-16 bottom-10 h-[85vh] w-auto object-contain opacity-30 brightness-0 invert drop-shadow-[0_4px_10px_rgba(255,255,255,0.1)]"
        />
      </div>

      <div className="absolute bottom-0 left-0 w-full pointer-events-none z-0">
        <img 
          src="/img/grass-footer.png" 
          alt="" 
          className="w-full h-32 md:h-48 object-cover object-bottom opacity-40 brightness-0 invert"
        />
      </div>

      <div className="absolute top-0 right-1/4 w-[600px] h-[600px] bg-white/10 rounded-full blur-[150px] pointer-events-none z-0"></div>
      <div className="absolute bottom-0 left-1/4 w-[600px] h-[600px] bg-blue-900/10 rounded-full blur-[150px] pointer-events-none z-0"></div>

      <div className="relative z-10 w-full max-w-md mt-[-5vh]">
        
        <div className="text-center mb-6">
          <h1 className="text-3xl font-extrabold text-white tracking-tight shadow-black/10 drop-shadow-md">
            Nexus Open-Logic
          </h1>
          <p className="text-white/80 text-sm font-medium mt-1">
            Gestão Integrada de Fluidos e Cultura
          </p>
        </div>

        <div className="bg-white rounded-2xl shadow-[0_20px_50px_rgba(0,0,0,0.25)] p-8 sm:p-10 border border-white/30 backdrop-blur-sm">
          
          <div className="flex flex-col items-center mb-6">
            <div className="w-16 h-16 mb-4">
              <img 
                src="/logo.png" 
                alt="Nexus Open-Logic" 
                className="w-full h-full object-contain drop-shadow-sm"
              />
            </div>
            <span className="text-[10px] font-bold uppercase tracking-widest text-blue-800 bg-blue-50 px-2.5 py-1 rounded border border-blue-100 mb-2">
              Acesso Restrito
            </span>
            <h2 className="text-xl font-bold text-slate-800 tracking-tight">Portal do Operador</h2>
          </div>

          <form onSubmit={handleLogin} className="space-y-5">
            <div>
              <label className="block text-sm font-semibold text-slate-700 mb-1.5" htmlFor="matricula">
                Matrícula (Login)
              </label>
              <input
                id="matricula"
                type="text"
                value={matricula}
                onChange={(e) => setMatricula(e.target.value)}
                disabled={isLoading}
                className="w-full px-4 py-3 rounded-lg border border-slate-300 bg-slate-50 text-slate-900 placeholder-slate-400 focus:outline-none focus:border-blue-500 focus:ring-4 focus:ring-blue-500/10 transition-all disabled:opacity-50 font-medium text-sm"
                placeholder="admin"
                required
              />
            </div>

            <div>
              <label className="block text-sm font-semibold text-slate-700 mb-1.5" htmlFor="password">
                Chave de Segurança
              </label>
              <input
                id="password"
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                disabled={isLoading}
                className="w-full px-4 py-3 rounded-lg border border-slate-300 bg-slate-50 text-slate-900 placeholder-slate-400 focus:outline-none focus:border-blue-500 focus:ring-4 focus:ring-blue-500/10 transition-all disabled:opacity-50 font-medium text-sm"
                placeholder="••••••••"
                required
              />
            </div>

            {errorMsg && (
              <div className="p-3 bg-red-50 text-red-700 text-sm font-medium rounded-lg border border-red-100 text-center">
                {errorMsg}
              </div>
            )}

            <button
              type="submit"
              disabled={isLoading}
              className="w-full py-3.5 px-4 bg-blue-600 hover:bg-blue-700 active:bg-blue-800 text-white font-bold tracking-wide rounded-lg shadow-md shadow-blue-600/20 transition-all flex items-center justify-center disabled:bg-slate-300 disabled:shadow-none disabled:text-slate-500 disabled:cursor-not-allowed mt-6 text-sm"
            >
              {isLoading ? (
                <svg className="animate-spin h-5 w-5 text-white" viewBox="0 0 24 24" fill="none">
                  <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                  <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                </svg>
              ) : (
                'Autenticar Sessão'
              )}
            </button>
          </form>
        </div>
      </div>
    </div>
  );
}