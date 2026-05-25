import Head from 'next/head';
import { useState } from 'react';
import { useRouter } from 'next/router';
import { sql } from '@vercel/postgres';
import Layout from '../components/Layout';

export default function GestaoUsuarios({ usuariosIniciais }) {
  const router = useRouter();
  const [searchTerm, setSearchTerm] = useState('');
  const [isModalOpen, setIsModalOpen] = useState(false);
  const [isSubmitting, setIsSubmitting] = useState(false);
  
  // Estado para saber se estamos editando (guarda o ID) ou criando novo (guarda null)
  const [usuarioEditandoId, setUsuarioEditandoId] = useState(null);

  const [formUsuario, setFormUsuario] = useState({
    matricula: '', nome: '', email: '', setor: 'Produção', rfid_id: '', limite_encher: '', limite_esvaziar: ''
  });

  const handleInputChange = (e) => {
    const { id, value } = e.target;
    setFormUsuario(prev => ({ ...prev, [id]: value }));
  };

  // Função para abrir o modal no modo "Novo Cadastro"
  const handleAbrirNovoModal = () => {
    setUsuarioEditandoId(null);
    setFormUsuario({ matricula: '', nome: '', email: '', setor: 'Produção', rfid_id: '', limite_encher: '', limite_esvaziar: '' });
    setIsModalOpen(true);
  };

  // Função para abrir o modal no modo "Edição"
  const handleEditarUsuario = (user) => {
    setUsuarioEditandoId(user.id);
    setFormUsuario({
      matricula: user.matricula, // Bloquearemos a edição da matrícula no input
      nome: user.nome,
      email: user.email,
      setor: user.setor,
      rfid_id: user.rfid_id || '',
      limite_encher: user.limite_encher,
      limite_esvaziar: user.limite_esvaziar
    });
    setIsModalOpen(true);
  };

  const handleSalvarUsuario = async (e) => {
    e.preventDefault();
    setIsSubmitting(true);

    // Se tiver ID, é edição (PUT). Se não tiver, é novo (POST).
    const method = usuarioEditandoId ? 'PUT' : 'POST';
    
    // Adicionamos o ID no body se for edição
    const bodyData = usuarioEditandoId ? { id: usuarioEditandoId, ...formUsuario } : formUsuario;

    const res = await fetch('/api/usuarios', {
      method: method,
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(bodyData)
    });

    setIsSubmitting(false);

    if (res.ok) {
      setIsModalOpen(false);
      router.replace(router.asPath); // Refresca a tabela com os novos dados
    } else {
      alert('Erro ao salvar. Verifique se os dados estão corretos ou se já existem.');
    }
  };

  const handleDeletarUsuario = async (id) => {
    if (confirm('Tem certeza que deseja excluir este usuário? O acesso dele será revogado imediatamente.')) {
      const res = await fetch('/api/usuarios', {
        method: 'DELETE',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id })
      });

      if (res.ok) {
        router.replace(router.asPath); 
      }
    }
  };

  // Filtro de busca na tabela (Pesquisa local)
  const usuariosFiltrados = usuariosIniciais.filter(user => 
    user.nome.toLowerCase().includes(searchTerm.toLowerCase()) || 
    user.matricula.toLowerCase().includes(searchTerm.toLowerCase())
  );

  return (
    <Layout setorUsuario="Gestão">
      <Head>
        <title>Gestão de Usuários - Nexus</title>
      </Head>

      <div className="p-6 md:p-10 max-w-7xl mx-auto w-full space-y-6">
        <div className="flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4 bg-white p-6 rounded-xl shadow-sm border border-slate-200">
          <div>
            <div className="flex items-center gap-2 mb-1">
              <span className="bg-purple-100 text-purple-700 text-[10px] font-bold uppercase tracking-wider px-2 py-0.5 rounded border border-purple-200">
                Acesso Restrito: Gestão
              </span>
            </div>
            <h1 className="text-2xl font-bold text-slate-800 tracking-tight">Gestão de Usuários</h1>
            <p className="text-sm text-slate-500 mt-1">Controle de acessos, tags RFID e cotas de volume físico.</p>
          </div>
          <button 
            onClick={handleAbrirNovoModal}
            className="bg-blue-600 hover:bg-blue-700 text-white px-5 py-2.5 rounded-lg font-medium transition-colors shadow-sm shadow-blue-600/20 flex items-center gap-2 text-sm shrink-0"
          >
            <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M12 4v16m8-8H4"></path></svg>
            Adicionar Usuário
          </button>
        </div>

        <div className="bg-white rounded-xl shadow-sm border border-slate-200 overflow-hidden">
          <div className="p-4 border-b border-slate-100 flex flex-col sm:flex-row gap-4 justify-between items-center bg-slate-50/50">
            <div className="relative w-full max-w-md">
              <svg className="absolute left-3 top-1/2 -translate-y-1/2 w-5 h-5 text-slate-400" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z"></path></svg>
              <input 
                type="text" 
                placeholder="Buscar por matrícula ou nome..." 
                className="w-full pl-10 pr-4 py-2 rounded-lg border border-slate-300 bg-white text-sm focus:outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-colors"
                value={searchTerm}
                onChange={(e) => setSearchTerm(e.target.value)}
              />
            </div>
          </div>

          <div className="overflow-x-auto">
            <table className="w-full text-left border-collapse min-w-[900px]">
              <thead>
                <tr className="bg-slate-50 text-slate-500 text-xs uppercase tracking-wider font-semibold border-b border-slate-200">
                  <th className="p-4 pl-6">Colaborador</th>
                  <th className="p-4">Setor</th>
                  <th className="p-4">Tag RFID</th>
                  <th className="p-4">Cota Máx. Encher</th>
                  <th className="p-4">Cota Máx. Esvaziar</th>
                  <th className="p-4 pr-6 text-right">Ações</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100 text-sm text-slate-700">
                {usuariosFiltrados.length === 0 ? (
                  <tr><td colSpan="6" className="p-8 text-center text-slate-500">Nenhum usuário encontrado.</td></tr>
                ) : usuariosFiltrados.map((user) => (
                  <tr key={user.id} className="hover:bg-slate-50/80 transition-colors">
                    <td className="p-4 pl-6">
                      <div className="font-semibold text-slate-900">{user.nome}</div>
                      <div className="text-xs text-slate-500 mt-0.5">{user.matricula} • {user.email}</div>
                    </td>
                    <td className="p-4">
                      <span className={`inline-flex px-2.5 py-1 rounded-md text-xs font-semibold border ${
                        user.setor === 'Gestão' ? 'bg-purple-50 text-purple-700 border-purple-200' : 'bg-slate-100 text-slate-700 border-slate-200'
                      }`}>
                        {user.setor}
                      </span>
                    </td>
                    <td className="p-4">
                      <span className="font-mono text-xs text-slate-600 bg-slate-100 px-2 py-1 rounded border border-slate-200">
                        {user.rfid_id || 'Sem Tag'}
                      </span>
                    </td>
                    <td className="p-4 font-bold text-blue-700">{user.limite_encher} L</td>
                    <td className="p-4 font-bold text-emerald-700">{user.limite_esvaziar} L</td>
                    <td className="p-4 pr-6 text-right">
                      <div className="flex justify-end gap-2">
                        {/* BOTÃO DE EDITAR NOVO AQUI */}
                        <button 
                          onClick={() => handleEditarUsuario(user)} 
                          className="text-slate-400 hover:text-blue-600 transition-colors p-1.5" 
                          title="Editar Usuário"
                        >
                          <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M11 5H6a2 2 0 00-2 2v11a2 2 0 002 2h11a2 2 0 002-2v-5m-1.414-9.414a2 2 0 112.828 2.828L11.828 15H9v-2.828l8.586-8.586z"></path></svg>
                        </button>
                        <button 
                          onClick={() => handleDeletarUsuario(user.id)} 
                          className="text-slate-400 hover:text-red-600 transition-colors p-1.5" 
                          title="Excluir Usuário"
                        >
                          <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16"></path></svg>
                        </button>
                      </div>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </div>

      {/* Modal de Cadastro e Edição */}
      {isModalOpen && (
        <div className="fixed inset-0 bg-slate-900/60 backdrop-blur-sm flex items-center justify-center p-4 z-50">
          <div className="bg-white rounded-xl shadow-xl border border-slate-200 max-w-md w-full overflow-hidden">
            <div className="p-6 border-b border-slate-100 flex justify-between items-center bg-slate-50">
              {/* O título muda dependendo de qual ação estamos fazendo */}
              <h3 className="font-bold text-lg text-slate-900">
                {usuarioEditandoId ? 'Editar Usuário' : 'Cadastrar Usuário'}
              </h3>
              <button onClick={() => setIsModalOpen(false)} className="text-slate-400 hover:text-slate-600">
                <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M6 18L18 6M6 6l12 12"></path></svg>
              </button>
            </div>
            
            <form onSubmit={handleSalvarUsuario} className="p-6 space-y-4">
              <div className="grid grid-cols-2 gap-4">
                <div>
                  <label className="block text-xs font-semibold text-slate-700 mb-1" htmlFor="matricula">Matrícula (Login)</label>
                  {/* Desativamos o campo matrícula se estiver editando para manter a chave no BD segura */}
                  <input id="matricula" type="text" required disabled={!!usuarioEditandoId} value={formUsuario.matricula} onChange={handleInputChange} className="w-full border border-slate-300 rounded-lg p-2 text-sm focus:outline-none focus:border-blue-500 disabled:bg-slate-100 disabled:text-slate-500" placeholder="NEX-000"/>
                </div>
                <div>
                  <label className="block text-xs font-semibold text-slate-700 mb-1" htmlFor="setor">Setor</label>
                  <select id="setor" value={formUsuario.setor} onChange={handleInputChange} className="w-full border border-slate-300 rounded-lg p-2 text-sm bg-white focus:outline-none focus:border-blue-500">
                    <option value="Produção">Produção</option>
                    <option value="Manutenção">Manutenção</option>
                    <option value="Gestão">Gestão</option>
                  </select>
                </div>
              </div>

              <div>
                <label className="block text-xs font-semibold text-slate-700 mb-1" htmlFor="nome">Nome Completo</label>
                <input id="nome" type="text" required value={formUsuario.nome} onChange={handleInputChange} className="w-full border border-slate-300 rounded-lg p-2 text-sm focus:outline-none focus:border-blue-500" placeholder="Nome do funcionário"/>
              </div>

              <div>
                <label className="block text-xs font-semibold text-slate-700 mb-1" htmlFor="email">E-mail</label>
                <input id="email" type="email" required value={formUsuario.email} onChange={handleInputChange} className="w-full border border-slate-300 rounded-lg p-2 text-sm focus:outline-none focus:border-blue-500" placeholder="nome@nexus.com"/>
              </div>

              <div>
                <label className="block text-xs font-semibold text-slate-700 mb-1" htmlFor="rfid_id">Tag RFID (Opcional - Físico)</label>
                <input id="rfid_id" type="text" value={formUsuario.rfid_id} onChange={handleInputChange} className="w-full border border-slate-300 rounded-lg p-2 text-sm font-mono uppercase focus:outline-none focus:border-blue-500" placeholder="Ex: A1:B2:C3:D4"/>
              </div>

              <div className="grid grid-cols-2 gap-4 p-3 bg-slate-50 rounded-lg border border-slate-200">
                <div>
                  <label className="block text-xs font-bold text-blue-700 mb-1" htmlFor="limite_encher">Cota Encher (L)</label>
                  <input id="limite_encher" type="number" step="0.1" required value={formUsuario.limite_encher} onChange={handleInputChange} className="w-full border border-slate-300 rounded-lg p-2 text-sm focus:outline-none focus:border-blue-500" placeholder="Max: 2.0" min="0"/>
                </div>
                <div>
                  <label className="block text-xs font-bold text-emerald-700 mb-1" htmlFor="limite_esvaziar">Cota Esvaziar (L)</label>
                  <input id="limite_esvaziar" type="number" step="0.1" required value={formUsuario.limite_esvaziar} onChange={handleInputChange} className="w-full border border-slate-300 rounded-lg p-2 text-sm focus:outline-none focus:border-blue-500" placeholder="Max: 2.0" min="0"/>
                </div>
              </div>

              {!usuarioEditandoId && (
                <div className="text-[10px] text-slate-500 bg-amber-50 p-2 rounded border border-amber-100">
                  * A senha inicial para novos acessos web será automaticamente: <b>nexus123</b>.
                </div>
              )}

              <div className="pt-4 flex justify-end gap-2 border-t border-slate-100">
                <button type="button" onClick={() => setIsModalOpen(false)} className="px-4 py-2 border border-slate-300 text-slate-700 text-sm rounded-lg hover:bg-slate-50 transition-colors">Cancelar</button>
                <button type="submit" disabled={isSubmitting} className="px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white text-sm font-semibold rounded-lg shadow-sm transition-colors disabled:opacity-50">
                  {isSubmitting ? 'Salvando...' : 'Salvar'}
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </Layout>
  );
}

export async function getServerSideProps() {
  try {
    const result = await sql`SELECT * FROM usuarios ORDER BY id DESC`;
    const usuariosIniciais = JSON.parse(JSON.stringify(result.rows));
    return {
      props: { usuariosIniciais },
    };
  } catch (error) {
    console.error('Erro ao conectar no banco:', error);
    return { props: { usuariosIniciais: [] } };
  }
}