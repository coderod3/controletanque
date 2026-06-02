import Link from "next/link";
import { useRouter } from "next/router";
import { useTankStore } from "../store/useTankStore";

const ALL_NAV_ITEMS = [
  { 
    name: "Painel de Telemetria", 
    href: "/dashboard", 
    icon: "M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zM14 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zM14 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z",
    roles: ["visualizacao", "operador", "gestor"]
  },
  { 
    name: "Logs & Auditoria", 
    href: "/logs", 
    icon: "M9 17v-2m3 2v-4m3 4v-6m2 10H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z",
    roles: ["operador", "gestor"]
  },
  { 
    name: "Gestão de Usuários", 
    href: "/usuarios", 
    icon: "M12 4.354a4 4 0 110 5.292M15 21H3v-1a6 6 0 0112 0v1zm0 0h6v-1a6 6 0 00-9-5.197M13 7a4 4 0 11-8 0 4 4 0 018 0z",
    roles: ["gestor"]
  },
  { 
    name: "Parametrização", 
    href: "/config", 
    icon: "M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z M15 12a3 3 0 11-6 0 3 3 0 016 0z",
    roles: ["gestor"]
  },
];

export default function Sidebar({ isOpen, activeRoute }) {
  const router = useRouter();
  const userRole = useTankStore((state) => state.userRole);

  const allowedItems = ALL_NAV_ITEMS.filter(item => item.roles.includes(userRole));

  const handleLogout = () => {
    // Aqui você pode limpar localStorages ou cookies futuramente se precisar
    useTankStore.getState().setUserRole(''); // Limpa o cargo atual
    document.cookie = "nexus_role=; path=/; expires=Thu, 01 Jan 1770 00:00:00 UTC;";
    router.push("/"); // Redireciona para o login
  };

  return (
    <aside className={`fixed md:relative z-30 inset-y-0 left-0 h-screen bg-slate-900 border-r border-slate-800 transition-all duration-300 overflow-hidden ${isOpen ? "w-64 translate-x-0" : "w-64 -translate-x-full md:w-0 md:translate-x-0"}`}>
      <div className="w-64 h-full flex flex-col">
        {/* Brand */}
        <div className="p-5 border-b border-slate-800 flex items-center gap-3 shrink-0">
          <div className="w-8 h-8 bg-blue-600 rounded-lg flex items-center justify-center shadow-md shadow-blue-900/40">
            <svg className="w-5 h-5 text-white" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19.428 15.428a2 2 0 00-1.022-.547l-2.387-.477a6 6 0 00-3.86.517l-.318.158a6 6 0 01-3.86.517L6.05 15.21a2 2 0 00-1.806.547M8 4h8l-1 1v5.172a2 2 0 00.586 1.414l5 5c1.26 1.26.367 3.414-1.415 3.414H4.828c-1.782 0-2.674-2.154-1.414-3.414l5-5A2 2 0 009 10.172V5L8 4z" /></svg>
          </div>
          <div>
            <span className="text-white font-bold tracking-wide text-sm block">Revita OS</span>
            <span className="text-slate-500 text-[10px] uppercase tracking-wider font-semibold">Cargo: {userRole}</span>
          </div>
        </div>

        {/* Dynamic Nav */}
        <nav className="flex-1 p-3 space-y-1 text-sm font-medium overflow-y-auto">
          {allowedItems.map((item) => {
            const isActive = activeRoute === item.href;
            return (
              <Link key={item.name} href={item.href} className={`flex items-center gap-3 px-4 py-2.5 rounded-lg transition-colors ${isActive ? "bg-blue-600/10 text-blue-400 border border-blue-500/20" : "text-slate-400 hover:bg-slate-800 hover:text-white"}`}>
                <svg className="w-5 h-5 shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d={item.icon} /></svg>
                {item.name}
              </Link>
            );
          })}
        </nav>

        {/* Footer */}
        <div className="p-3 border-t border-slate-800 shrink-0">
          <button onClick={handleLogout} className="flex items-center gap-3 px-4 py-2 w-full text-sm font-medium text-slate-400 hover:text-red-400 hover:bg-red-500/10 rounded-lg transition-colors">
            <svg className="w-5 h-5 shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1" /></svg>
            Encerrar Sessão
          </button>
        </div>
      </div>
    </aside>
  );
}