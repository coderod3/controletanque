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
    res.status(200).json({ message: "Connection successful" });
    client.release();
  } catch (error) {
    console.error(error);
    res.status(500).json({ error: "Connection failed" });
  }
}
