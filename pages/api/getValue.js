const { Pool } = require('pg');

const pool = new Pool({
  user: 'default',
  host: 'ep-patient-poetry-a49trlr1-pooler.us-east-1.aws.neon.tech',
  database: 'verceldb',
  password: 'pvyQPOlU78nH',
  port: 5432,
  ssl: {
    rejectUnauthorized: false
  }
});

export default async function handler(req, res) {
  try {
    const client = await pool.connect();
    const result = await client.query('SELECT col1 FROM valores LIMIT 1');
    client.release();

    if (result.rows.length > 0) {
      const value = result.rows[0].col1;
      res.status(200).json({ value });
    } else {
      res.status(404).json({ error: "No data found" });
    }
  } catch (error) {
    console.error(error);
    res.status(500).json({ error: "Internal Server Error" });
  }
}