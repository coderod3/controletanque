import { db } from '@vercel/postgres';

export default async function handler(req, res) {
  if (req.method !== 'GET') {
    return res.status(405).json({ error: 'Método não permitido' });
  }

  const client = await db.connect();

  try {
    // Selecionamos os campos necessários para a auditoria
    // u.nome vem da tabela de usuários através da chave estrangeira usuario_id
    const result = await client.sql`
      SELECT 
        h.id,
        u.nome as operador,
        h.evento,
        h.valor_anterior,
        h.valor_atual,
        h.status,
        h.hora_request_recebido_servidor as data
      FROM historico h
      LEFT JOIN usuarios u ON h.usuario_id = u.id
      ORDER BY h.hora_request_recebido_servidor DESC
      LIMIT 50;
    `;

    return res.status(200).json(result.rows);
  } catch (error) {
    console.error("Erro na API de Histórico:", error);
    return res.status(500).json({ error: error.message });
  } finally {
    client.release();
  }
}