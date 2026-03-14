require('dotenv').config({ path: '.env.development.local' });
const { Pool } = require('pg');

const pool = new Pool({
  connectionString: process.env.POSTGRES_URL,
  ssl: { rejectUnauthorized: false }
});

export default async function handler(req, res) {
  const client = await pool.connect();
  const { id_tanque, nivel, data_placa, intervalo, inicio, fim, format, ordem } = req.query;
  const direcao = ordem === 'asc' ? 'ASC' : 'DESC';

  try {
    // --- LÓGICA DE INSERÇÃO (Arduino/ESP32) ---
    if (id_tanque && nivel && data_placa) {
      const previousResult = await client.query(
        'SELECT nivel_atual FROM historico_tanques WHERE id_tanque = $1 ORDER BY id DESC LIMIT 1',
        [parseInt(id_tanque)]
      );
      const nivelAnterior = previousResult.rows.length > 0 ? previousResult.rows[0].nivel_atual : null;

      await client.query(
        'INSERT INTO historico_tanques (id_tanque, data_hora_placa, nivel_anterior, nivel_atual) VALUES ($1, $2, $3, $4)',
        [parseInt(id_tanque), data_placa, nivelAnterior, parseFloat(nivel)]
      );
      return res.status(201).json({ message: "Log criado" });
    }

    // --- LÓGICA DE CONSULTA E FILTRO (CORRIGIDA) ---
    let filtros = [];
    const params = [];

    // 1. Filtro por ID do Tanque (Se não for "all")
    if (id_tanque && id_tanque !== 'all') {
      filtros.push(`id_tanque = $${params.length + 1}`);
      params.push(parseInt(id_tanque));
    }

    // 2. Filtro por Intervalo de Tempo
    if (intervalo === '1d') {
      filtros.push("data_hora_servidor > NOW() - INTERVAL '1 day'");
    } else if (intervalo === '1w') {
      filtros.push("data_hora_servidor > NOW() - INTERVAL '7 days'");
    } else if (intervalo === 'custom' && inicio && fim) {
      filtros.push(`data_hora_servidor BETWEEN $${params.length + 1} AND $${params.length + 2}`);
      params.push(inicio, fim);
    }

    // Constrói a cláusula WHERE dinamicamente
    const queryFiltro = filtros.length > 0 ? `WHERE ${filtros.join(' AND ')}` : "";

    const selectQuery = `
      SELECT id_tanque, 
            TO_CHAR(data_hora_placa, 'DD/MM/YYYY HH24:MI:SS') as data_placa, 
            TO_CHAR(data_hora_servidor AT TIME ZONE 'UTC' AT TIME ZONE 'America/Sao_Paulo', 'DD/MM/YYYY HH24:MI:SS') as data_servidor,
            nivel_anterior, nivel_atual 
      FROM historico_tanques 
      ${queryFiltro}
      ORDER BY data_hora_placa ${direcao}
      LIMIT 500
    `;

    const result = await client.query(selectQuery, params);

    // --- LÓGICA DE RESPOSTA (JSON ou CSV) ---
    if (format === 'csv') {
      const cabecalho = "ID Tanque,Data Placa,Data Servidor,Nivel Anterior (L),Nivel Atual (L)\n";
      const linhas = result.rows.map(r => 
        `${r.id_tanque},${r.data_placa},${r.data_servidor},${r.nivel_anterior || 0},${r.nivel_atual}`
      ).join("\n");
      
      res.setHeader('Content-Type', 'text/csv');
      res.setHeader('Content-Disposition', 'attachment; filename=historico_tanque.csv');
      return res.status(200).send(cabecalho + linhas);
    }

    // Padrão: Retorna JSON para a tabela
    res.status(200).json(result.rows);

  } catch (error) {
    res.status(500).json({ error: error.message });
  } finally {
    client.release();
  }
}