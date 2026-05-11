import { useEffect, useState } from 'react';
import Link from 'next/link';

export default function LogsPage() {
  const [logs, setLogs] = useState([]);
  const [loading, setLoading] = useState(true);

  const carregarDados = async () => {
    try {
      const res = await fetch('/api/telemetria/historico');
      const data = await res.json();
      setLogs(data);
    } catch (err) {
      console.error("Erro ao carregar logs:", err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    carregarDados();
  }, []);

  return (
    <div style={{ padding: '20px', fontFamily: 'sans-serif' }}>
      <header style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <h1>Histórico de Operações (Audit Log Completo)</h1>
        <div>
            <button onClick={carregarDados} style={{ marginRight: '10px', padding: '8px 16px', cursor: 'pointer' }}>Atualizar</button>
            <Link href="/dashboard"><button style={{ padding: '8px 16px', cursor: 'pointer' }}>Voltar ao Painel</button></Link>
        </div>
      </header>

      <hr style={{ margin: '20px 0' }} />

      {loading ? (
        <p>Carregando registros...</p>
      ) : (
        <div style={{ overflowX: 'auto', width: '100%', borderRadius: '8px', border: '1px solid #e2e8f0' }}>
          <table border="1" cellPadding="12" style={{ width: '100%', borderCollapse: 'collapse', border: 'none', fontSize: '0.9rem', whiteSpace: 'nowrap' }}>
            <thead style={{ backgroundColor: '#f8fafc', textAlign: 'left' }}>
              <tr>
                  <th>Data/Hora</th>
                  <th>ID Registro</th>
                  <th>Origem</th>
                  <th>Operador</th>
                  <th>Evento / Ação</th>
                  <th>Vol. Pedido</th>
                  <th>Nível Inicial</th>
                  <th>Nível Final</th>
                  <th>Variação</th>
                  <th>Gestor Aut.</th>
                  <th>Status</th>
              </tr>
            </thead>
            <tbody>
              {logs.map((log) => {
                const variacao = (log.valor_atual - log.valor_anterior).toFixed(2);
                const corVariacao = variacao > 0 ? '#16a34a' : (variacao < 0 ? '#dc2626' : '#475569');
                // Corta o UUID para não ocupar muito espaço (ex: 123e4567...)
                const idCurto = log.id ? log.id.split('-')[0] : 'N/A';

                return (
                  <tr key={log.id} style={{ borderBottom: '1px solid #e2e8f0' }}>
                    <td>{new Date(log.data).toLocaleString('pt-BR')}</td>
                    <td title={log.id} style={{ fontFamily: 'monospace', color: '#64748b' }}>#{idCurto}</td>
                    <td><strong>{log.origem_comando || 'HARDWARE'}</strong></td>
                    <td>{log.operador}</td>
                    <td>{log.evento}</td>
                    <td>{log.valor_recebido != null ? `${log.valor_recebido}L` : '-'}</td>
                    <td>{log.valor_anterior}L</td>
                    <td>{log.valor_atual}L</td>
                    <td style={{ color: corVariacao, fontWeight: 'bold' }}>
                      {variacao > 0 ? '+' : ''}{variacao}L
                    </td>
                    <td style={{ fontSize: '0.8rem', color: '#64748b' }}>{log.gestor_autorizador_id || 'Nenhum'}</td>
                    <td>
                      <span style={{ 
                        padding: '4px 8px', 
                        borderRadius: '4px', 
                        fontSize: '0.8rem',
                        fontWeight: 'bold',
                        backgroundColor: log.status === 'SUCESSO' ? '#dcfce7' : '#fee2e2',
                        color: log.status === 'SUCESSO' ? '#16a34a' : '#dc2626'
                      }}>
                        {log.status}
                      </span>
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}