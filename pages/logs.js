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
        <h1>Histórico de Operações (Audit Log)</h1>
        <div>
            <button onClick={carregarDados} style={{ marginRight: '10px' }}>Atualizar</button>
            <Link href="/dashboard"><button>Voltar ao Painel</button></Link>
        </div>
      </header>

      <hr />

      {loading ? (
        <p>Carregando registros...</p>
      ) : (
        <table border="1" cellPadding="10" style={{ width: '100%', borderCollapse: 'collapse', marginTop: '20px' }}>
          <thead style={{ backgroundColor: '#f4f4f4' }}>
            <tr>
              <th>Data/Hora</th>
              <th>Operador</th>
              <th>Evento</th>
              <th>Nível Inicial</th>
              <th>Nível Final</th>
              <th>Variação Líquida</th>
              <th>Status</th>
            </tr>
          </thead>
          <tbody>
            {logs.map((log) => {
              const variacao = (log.valor_atual - log.valor_anterior).toFixed(2);
              const corVariacao = variacao > 0 ? 'green' : (variacao < 0 ? 'red' : 'black');

              return (
                <tr key={log.id}>
                  <td>{new Date(log.data).toLocaleString('pt-BR')}</td>
                  <td>{log.operador || 'Sistema / Hardware'}</td>
                  <td>{log.evento}</td>
                  <td>{log.valor_anterior}L</td>
                  <td>{log.valor_atual}L</td>
                  <td style={{ color: corVariacao, fontWeight: 'bold' }}>
                    {variacao > 0 ? '+' : ''}{variacao}L
                  </td>
                  <td style={{ color: log.status === 'SUCESSO' ? 'green' : 'red' }}>
                    {log.status}
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      )}
    </div>
  );
}