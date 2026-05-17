import { db } from '@vercel/postgres';

export default async function handler(req, res) {
  if (req.method !== 'GET') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  const client = await db.connect();

  try {
    // Busca todos os usuários que têm uma tag RFID associada
    const { rows } = await client.sql`
      SELECT id, rfid_uid, nome 
      FROM usuarios 
      WHERE rfid_uid IS NOT NULL AND rfid_uid != ''
    `;
    return res.status(200).json(rows);
  } catch (error) {
    console.error("Erro ao buscar usuários:", error);
    return res.status(500).json({ error: error.message });
  } finally {
    client.release();
  }
}