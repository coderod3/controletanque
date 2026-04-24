import pool from '../../../lib/db';

export default async function handler(req, res) {
  if (req.method !== 'POST' && req.method !== 'GET') {
    return res.status(405).json({ error: 'Método não permitido' });
  }

  // A placa enviará: id_tanque, distancia (cm), evento, rfid_uid (opcional)
  const { id_tanque, distancia, evento, rfid_uid } = req.query;

  if (!id_tanque || !distancia) {
    return res.status(400).json({ error: 'Dados insuficientes da placa' });
  }

  const client = await pool.connect();

  try {
    const distFloat = parseFloat(distancia);
    
    // 1. Cálculo da escala industrial (18cm = 0L | 2cm = 100L)
    const volumeCalculado = Math.max(0, Math.min(100, (18.0 - distFloat) * 6.25));

    // 2. Busca o volume anterior para o log
    const prevRes = await client.query(
      'SELECT nivel_atual FROM tanques WHERE id = $1', [id_tanque]
    );
    const volumeAnterior = prevRes.rows.length > 0 ? prevRes.rows[0].nivel_atual : 0;

    // 3. Atualiza o Gêmeo Digital (Tabela tanques)
    await client.query(
      'UPDATE tanques SET nivel_atual = $1, ultima_sincronizacao = NOW() WHERE id = $2',
      [volumeCalculado, id_tanque]
    );

    // 4. Insere no Histórico para o BI
    await client.query(
      `INSERT INTO historico 
       (tanque_id, evento, origem, valor_recebido, valor_anterior, valor_atual, hora_request_recebido_servidor) 
       VALUES ($1, $2, $3, $4, $5, $6, NOW())`,
      [id_tanque, evento || 'MOVIMENTACAO', rfid_uid ? 'RFID' : 'WEB', volumeCalculado, volumeAnterior, volumeCalculado]
    );

    res.status(200).json({ status: 'Sincronizado', volume: volumeCalculado });
  } catch (error) {
    res.status(500).json({ error: error.message });
  } finally {
    client.release();
  }
}