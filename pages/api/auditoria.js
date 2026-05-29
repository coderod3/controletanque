import pool from '../../../lib/db'; // Ajuste os '../' se o ficheiro estiver em pages/api/auditoria.js

export default async function handler(req, res) {
  // Apenas aceitamos métodos POST (O ESP32 envia HTTP POST)
  if (req.method !== 'POST') {
    return res.status(405).json({ error: 'Método não permitido. Utilize POST.' });
  }

  try {
    const payload = req.body;

    // 1. Capturar informações da rede/servidor (Útil se o comando vier do site web)
    const user_ip = req.headers['x-forwarded-for'] || req.socket.remoteAddress || 'IP_DESCONHECIDO';
    const user_agent = req.headers['user-agent'] || 'ESP32_HARDWARE';

    // 2. Extrair dados do JSON
    const {
      tanque_id,
      usuario_id,
      tipo_operacao,
      origem_comando,
      status,
      fisica,
      timestamps
    } = payload;

    // 3. Converter os tempos Epoch (segundos) enviados pela placa para formato Timestamp
    // A multiplicação * 1000 converte Segundos para Milissegundos no Javascript
    const ts_recebido_placa = new Date(timestamps.recebido_placa * 1000).toISOString();
    const ts_inicio_execucao = new Date(timestamps.inicio_execucao * 1000).toISOString();
    const ts_fim_execucao = new Date(timestamps.fim_execucao * 1000).toISOString();

    // 4. Inserir na base de dados
    const query = `
      INSERT INTO logs (
        tanque_id,
        usuario_id,
        tipo_operacao,
        origem_comando,
        status,
        volume_alvo,
        volume_inicial,
        volume_final,
        duracao_ms,
        ts_recebido_placa,
        ts_inicio_execucao,
        ts_fim_execucao,
        user_ip,
        user_agent
      ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14)
      RETURNING id;
    `;

    const values = [
      tanque_id || 'NEXUS_TANK_01',
      usuario_id || 'DESCONHECIDO',
      tipo_operacao,
      origem_comando,
      status,
      fisica.volume_alvo,
      fisica.volume_inicial,
      fisica.volume_final,
      fisica.duracao_ms,
      ts_recebido_placa,
      ts_inicio_execucao,
      ts_fim_execucao,
      user_ip,
      user_agent
    ];

    const result = await pool.query(query, values);

    // 5. Retornar SUCESSO (Status 201)
    // O retorno deste status 201 (Created) é o que avisa a tarefa do ESP32
    // para retirar este log específico da memória RAM (QueueReceive).
    return res.status(201).json({ 
        success: true, 
        message: 'Log gravado com sucesso', 
        id: result.rows[0].id 
    });

  } catch (error) {
    console.error('[AUDITORIA API] Erro ao gravar log:', error);
    return res.status(500).json({ error: 'Erro interno no servidor ao gravar o registo.' });
  }
}