// Carrega variáveis de ambiente (necessário para testes locais, no Vercel é automático)
require('dotenv').config({ path: '.env.development.local' });

const { Pool } = require('pg');

// Configuração da conexão seguindo seu padrão atual
const pool = new Pool({
  connectionString: process.env.POSTGRES_URL,
  ssl: {
    rejectUnauthorized: false
  }
});

export default async function handler(req, res) {
  const client = await pool.connect();

  try {
    const { id_tanque, nivel, data_placa } = req.query;

    // --- LÓGICA DE SET (Inserir novo log) ---
    // Verifica se os parâmetros necessários foram enviados pela placa ou site
    if (id_tanque && nivel && data_placa) {
      const idTanqueNum = parseInt(id_tanque);
      const novoNivel = parseFloat(nivel);

      // Busca o nível anterior do banco para este tanque específico (igual ao logsEp.js)
      const previousResult = await client.query(
        'SELECT nivel_atual FROM historico_tanques WHERE id_tanque = $1 ORDER BY id DESC LIMIT 1',
        [idTanqueNum]
      );
      
      const nivelAnterior = previousResult.rows.length > 0 
        ? previousResult.rows[0].nivel_atual 
        : null;

      // Insere na nova tabela com data da placa e data do servidor automática
      const insertQuery = `
        INSERT INTO historico_tanques (id_tanque, data_hora_placa, nivel_anterior, nivel_atual)
        VALUES ($1, $2, $3, $4)
      `;
      
      await client.query(insertQuery, [idTanqueNum, data_placa, nivelAnterior, novoNivel]);

      return res.status(201).json({ message: "Log histórico criado com sucesso" });
    }

    // --- LÓGICA DE GET (Ler logs para a tabela) ---
    // Se não houver parâmetros de inserção, retorna os últimos 50 registros
    const selectQuery = `
      SELECT id, id_tanque, 
             TO_CHAR(data_hora_placa, 'DD/MM/YYYY HH24:MI:SS') as data_placa, 
             TO_CHAR(data_hora_servidor, 'DD/MM/YYYY HH24:MI:SS') as data_servidor,
             nivel_anterior, nivel_atual 
      FROM historico_tanques 
      ORDER BY data_hora_servidor DESC 
      LIMIT 50
    `;
    
    const result = await client.query(selectQuery);
    res.status(200).json(result.rows);

  } catch (error) {
    console.error('Erro na API de histórico:', error);
    res.status(500).json({ error: "Erro interno no servidor" });
  } finally {
    // Importante liberar o cliente para não esgotar as conexões do banco
    client.release();
  }
}