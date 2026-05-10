import { db } from '@vercel/postgres';

export default async function handler(req, res) {
  // Garantimos que apenas POST seja aceito para proteger o dado
  if (req.method !== 'POST') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  const { tag_id } = req.body;
  console.log("TAG LIDa: ", tag_id); // <-- ADICIONE APENAS ESTA LINHA
  if (!tag_id) return res.status(400).json({ authorized: false, error: 'Tag ID ausente' });

  const client = await db.connect();

  try {
    // Consulta na tabela 'usuarios' que você criou no Neon
    const user = await client.sql`
      SELECT id, nome, cargo FROM usuarios WHERE rfid_uid = ${tag_id} LIMIT 1
    `;

    if (user.rows.length > 0) {
      return res.status(200).json({ 
        authorized: true, 
        nome: user.rows[0].nome,
        cargo: user.rows[0].cargo,
        db_id: user.rows[0].id
      });
    }

    return res.status(401).json({ authorized: false, error: 'Tag não cadastrada' });

  } catch (error) {
    console.error('Erro na Auth API:', error);
    return res.status(500).json({ error: 'Erro interno no servidor' });
  } finally {
    client.release();
  }
}