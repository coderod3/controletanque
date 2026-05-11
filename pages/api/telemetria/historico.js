import { db } from '@vercel/postgres';

export default async function handler(req, res) {
  if (req.method !== 'GET') return res.status(405).json({ error: 'Method not allowed' });

  const client = await db.connect();

  try {
    // Busca exata respeitando o schema fornecido
    const { rows } = await client.sql`
      SELECT 
        h.id,
        h.hora_request_recebido_servidor AS data, 
        COALESCE(u.nome, 'Sistema / Hardware') AS operador, 
        h.evento, 
        h.origem,
        h.origem_comando,
        h.valor_recebido,
        h.valor_anterior, 
        h.valor_atual, 
        h.status,
        COALESCE(g.nome, 'Nenhum') AS gestor
      FROM historico h
      LEFT JOIN usuarios u ON h.usuario_id = u.id
      LEFT JOIN usuarios g ON h.gestor_autorizador_id = g.id
      ORDER BY h.hora_request_recebido_servidor DESC 
      LIMIT 100
    `;
    return res.status(200).json(rows);
  } catch (error) {
    console.error("Erro fatal ao buscar histórico:", error);
    return res.status(500).json({ error: error.message });
  } finally {
    client.release();
  }
}