import { db } from '@vercel/postgres';

export default async function handler(req, res) {
  if (req.method !== 'GET') return res.status(405).json({ error: 'Method not allowed' });

  const client = await db.connect();

  try {
    const { rows } = await client.sql`
      SELECT 
        h.id,
        h.data, 
        COALESCE(u.nome, 'Sistema Autorizado') AS operador, 
        h.origem_comando,
        h.evento, 
        h.valor_recebido,
        h.valor_anterior, 
        h.valor_atual, 
        h.status,
        h.gestor_autorizador_id,
        h.tanque_id
      FROM historico h
      LEFT JOIN usuarios u ON h.usuario_id = u.id
      ORDER BY h.data DESC 
      LIMIT 100
    `;
    return res.status(200).json(rows);
  } catch (error) {
    console.error("Erro ao buscar histórico:", error);
    return res.status(500).json({ error: error.message });
  } finally {
    client.release();
  }
}