import { db } from '@vercel/postgres';

export default async function handler(req, res) {
  if (req.method !== 'POST') return res.status(405).json({ error: 'Method not allowed' });

  const { status, nivel } = req.body;
  const client = await db.connect();

  try {
    await client.sql`
      UPDATE tanques 
      SET status_operacional = ${status}, 
          nivel_atual = ${nivel}, 
          ultima_sincronizacao = NOW()
      WHERE nome = 'Tanque Principal'
    `;
    return res.status(200).json({ success: true });
  } catch (error) {
    console.error("Erro no Digital Twin:", error);
    return res.status(500).json({ error: error.message });
  } finally {
    client.release();
  }
}