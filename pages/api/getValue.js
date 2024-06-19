const { Pool } = require('pg');

const pool = new Pool({
  connectionString: process.env.DATABASE_URL,
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
