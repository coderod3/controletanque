import { useState, useEffect } from 'react';
import { useRouter } from 'next/router';
import Link from 'next/link';

export default function Layout({ children, setorUsuario = 'Gestão' }) {
  const router = useRouter();
  
  // No desktop, começa aberto. No mobile, começa fechado.
  const [isSidebarOpen, setIsSidebarOpen] = useState(true);

  // Fecha o menu automaticamente no mobile se o usuário trocar de página
  useEffect(() => {
    if (window.innerWidth < 768) {
      setIsSidebarOpen(false);
    }
  }, [router.pathname]);

  const handleLogout = () => {
    router.push('/');
  };

  const isActive = (path) => router.pathname === path;

  // =========================================================
  // LÓGICA DE PERMISSÕES (RBAC - Role-Based Access Control)
  // =========================================================
  const rotasDoSistema = [
    { 
      name: 'Painel de Telemetria', 
      path: '/dashboard', 
      icon: 'M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zM14 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zM14 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z',
      setoresPermitidos: ['Gestão', 'Manutenção', 'Produção'] // Todos veem o tanque
    },
    { 
      name: 'Gestão de Usuários', 
      path: '/usuarios', 
      icon: 'M12 4.354a4 4 0 110 5.292M15 21H3v-1a6 6 0 0112 0v1zm0 0h6v-1a6 6 0 00-9-5.197M13 7a4 4 0 11-8 0 4 4 0 018 0z',
      setoresPermitidos: ['Gestão'] // Apenas Administradores
    },
    { 
      name: 'Logs & Auditoria', 
      path: '/logs', 
      icon: 'M9 17v-2m3 2v-4m3 4v-6m2 10H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z',
      setoresPermitidos: ['Gestão', 'Manutenção'] // Operador de produção não audita
    },
    { 
      name: 'Parametrização do Tanque', 
      path: '/parametrizacao', 
      icon: 'M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z M15 12a3 3 0 11-6 0 3 3 0 016 0z',
      setoresPermitidos: ['Gestão', 'Manutenção'] // Apenas quem configura hardware
    }
  ];

  // Filtra as rotas para renderizar apenas o que o setor do usuário permite
  const rotasPermitidas = rotasDoSistema.filter(rota => rota.setoresPermitidos.includes(setorUsuario));

  return (
    <div className="min-h-screen bg-slate-50 flex overflow-hidden">
      
      {/* Overlay Escuro (Fecha o menu ao clicar fora no celular) */}
      <div 
        className={`fixed inset-0 bg-slate-900/60 z-20 md:hidden backdrop-blur-sm transition-opacity duration-300 ${isSidebarOpen ? 'opacity-100' : 'opacity-0 pointer-events-none'}`}
        onClick={() => setIsSidebarOpen(false)}
      />

      {/* ========================================================= */}
      {/* MENU LATERAL INDEPENDENTE (SIDEBAR)                         */}
      {/* ========================================================= */}
      <aside 
        className={`
          fixed md:relative z-30 inset-y-0 left-0 h-screen bg-slate-900 border-r border-slate-800 
          transition-all duration-300 ease-in-out overflow-hidden
          ${isSidebarOpen ? 'w-64 translate-x-0' : 'w-64 -translate-x-full md:w-0 md:translate-x-0'}
        `}
      >
        {/* Usamos w-64 interno para o texto não esmagar quando a barra recolhe */}
        <div className="w-64 h-full flex flex-col">
          <div className="p-6 border-b border-slate-800 flex items-center gap-3 shrink-0">
            <div className="w-8 h-8 bg-white rounded flex items-center justify-center p-1">
              <img src="/logo.png" alt="Nexus" className="w-full h-full object-contain" />
            </div>
            <span className="text-white font-bold tracking-wide text-sm">Revita Open-Logic</span>
          </div>

          <nav className="flex-1 p-4 space-y-2 text-sm font-medium overflow-y-auto">
            {/* Renderização Dinâmica e Segura das Rotas */}
            {rotasPermitidas.map((rota) => (
              <Link 
                key={rota.path} 
                href={rota.path} 
                className={`flex items-center gap-3 px-4 py-3 rounded-lg transition-colors ${isActive(rota.path) ? 'bg-blue-600/10 text-blue-400 border border-blue-500/20 shadow-inner' : 'text-slate-300 hover:bg-slate-800 hover:text-white'}`}
              >
                <svg className="w-5 h-5 shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d={rota.icon}></path>
                </svg>
                {rota.name}
              </Link>
            ))}
          </nav>

          <div className="p-4 border-t border-slate-800 shrink-0">
            <div className="mb-4 px-4 py-2 bg-slate-800/50 rounded-lg border border-slate-700/50">
              <p className="text-[10px] text-slate-400 uppercase font-bold tracking-wider mb-1">Setor Atual</p>
              <p className="text-sm text-white font-medium flex items-center gap-2">
                <span className="w-2 h-2 rounded-full bg-emerald-500"></span>
                {setorUsuario}
              </p>
            </div>
            <button onClick={handleLogout} className="flex items-center gap-3 px-4 py-2 w-full text-sm font-medium text-slate-400 hover:text-red-400 hover:bg-red-500/10 rounded-lg transition-colors">
              <svg className="w-5 h-5 shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1"></path></svg>
              Encerrar Sessão
            </button>
          </div>
        </div>
      </aside>

      {/* ========================================================= */}
      {/* CONTEÚDO DA PÁGINA                                        */}
      {/* ========================================================= */}
      <main className="flex-1 flex flex-col h-screen overflow-hidden">
        
        {/* TOPBAR INDEPENDENTE COM BOTÃO HAMBÚRGUER FIXO */}
        <header className="bg-white text-slate-800 p-4 border-b border-slate-200 flex items-center justify-between shrink-0 shadow-sm z-10">
          <div className="flex items-center gap-4">
            <button 
              onClick={() => setIsSidebarOpen(!isSidebarOpen)} 
              className="p-2 rounded-lg bg-slate-50 hover:bg-slate-100 border border-slate-200 transition-colors"
              title="Alternar Menu Lateral"
            >
              {/* Ícone muda se estiver aberto ou fechado */}
              <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                {isSidebarOpen 
                  ? <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 6h16M4 12h16M4 18h16"></path> 
                  : <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 6h16M4 12h8m-8 6h16"></path>
                }
              </svg>
            </button>
            <span className="font-bold tracking-tight text-lg text-slate-700 hidden sm:block">Painel Operacional</span>
          </div>
          
          <div className="flex items-center gap-3">
            <span className="text-xs font-semibold text-slate-500 bg-slate-100 px-3 py-1.5 rounded-full border border-slate-200">
              ID: {setorUsuario}
            </span>
          </div>
        </header>

        {/* Onde a página atual (como usuarios.jsx) vai renderizar */}
        <div className="flex-1 overflow-y-auto bg-slate-50">
          {children}
        </div>

      </main>
    </div>
  );
}