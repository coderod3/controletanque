require('dotenv').config({ path: '.env.development.local' });

const { Pool } = require('pg');

const pool = new Pool({
  connectionString: process.env.POSTGRES_URL,
  ssl: {
    rejectUnauthorized: false  // Only for development; do not use in production without proper SSL setup
  }
});

export default async function handler(req, res) {
  try {
    const client = await pool.connect();

    if (req.query.level) {
      const level = parseFloat(req.query.level); // Parse the level parameter to a float
      const updateQuery = 'UPDATE valores SET col2 = $1';
      await client.query(updateQuery, [level]);
      
      res.status(200).json({ level: level });
    } else {
      const selectQuery = 'SELECT col2 FROM valores';
      const result = await client.query(selectQuery);

      if (result.rows.length > 0) {
        const currentLevel = result.rows[0].col2; // Corrected from col1 to col2
        res.status(200).json({ level: currentLevel });
      } else {
        res.status(404).json({ error: "No level found" });
      }
    }

    client.release();
  } catch (error) {
    console.error(error);
    res.status(500).json({ error: "Internal Server Error" });
  }
}
