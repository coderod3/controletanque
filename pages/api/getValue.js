// Load environment variables from .env.development.local file
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
      // If ?level=<value> is provided in the URL, update the database
      const level = parseInt(req.query.level); // Parse the level parameter to an integer
      const updateQuery = 'UPDATE valores SET col1 = $1';
      await client.query(updateQuery, [level]);

      res.status(200).json({ message: `Level set to ${level}` });
    } else {
      // If no ?level=<value> is provided, retrieve the current level from the database
      const selectQuery = 'SELECT col1 FROM valores';
      const result = await client.query(selectQuery);

      if (result.rows.length > 0) {
        const currentLevel = result.rows[0].col1;
        res.status(200).json(currentLevel);
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
