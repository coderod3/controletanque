import { db } from '@vercel/postgres';

export default async function handler(req, res) {
  if (req.method !== 'GET') return res.status(405).json({ error: 'Method not allowed' });

  const client = await db.connect();

  try {
    // Agora a consulta é direta e sem aliases gambiarras
    const { rows } = await client.sql`
      SELECT 
        status_operacional, 
        nivel_atual, 
        volume_maximo, 
        distancia_vazio, 
        distancia_cheio 
      FROM tanques 
      LIMIT 1
    `;

    if (rows.length === 0) return res.status(404).json({ error: 'Nenhum tanque configurado' });

    return res.status(200).json(rows[0]);
  } catch (error) {
    console.error("Erro ao ler Digital Twin:", error);
    return res.status(500).json({ error: error.message });
  } finally {
    client.release();
  }
}