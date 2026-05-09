import { db } from '@vercel/postgres';

export default async function handler(req, res) {
  if (req.method !== 'POST') return res.status(405).json({ error: 'Method not allowed' });

  const { rfid_uid, acao, volume, status, valor_anterior, valor_atual } = req.body;
  const client = await db.connect();

  try {
    // 1. Busca o ID do usuário pelo RFID
    const userRes = await client.sql`
      SELECT id FROM usuarios WHERE rfid_uid = ${rfid_uid} LIMIT 1
    `;
    const usuario_id = userRes.rows[0]?.id || null;

    // 2. Busca o ID do tanque principal
    const tankRes = await client.sql`
      SELECT id FROM tanques WHERE nome = 'Tanque Principal' LIMIT 1
    `;
    const tanque_id = tankRes.rows[0]?.id || null;

    // 3. Insere a auditoria com todas as chaves estrangeiras resolvidas
    await client.sql`
      INSERT INTO historico (tanque_id, usuario_id, evento, origem, valor_recebido, valor_anterior, valor_atual, status)
      VALUES (${tanque_id}, ${usuario_id}, ${acao}, 'HARDWARE', ${volume}, ${valor_anterior}, ${valor_atual}, ${status})
    `;

    // 4. Atualiza o nível atual do tanque no Gêmeo Digital
    await client.sql`
      UPDATE tanques SET nivel_atual = ${valor_atual}, ultima_sincronizacao = NOW()
      WHERE nome = 'Tanque Principal'
    `;

    return res.status(200).json({ success: true });
  } catch (error) {
    console.error(error);
    return res.status(500).json({ error: error.message });
  } finally {
    client.release();
  }
}