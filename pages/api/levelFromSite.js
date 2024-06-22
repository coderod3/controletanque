// Load environment variables from .env.development.local file
require('dotenv').config({ path: '.env.development.local' });

const { Pool } = require('pg');

const pool = new Pool({
  connectionString: process.env.POSTGRES_URL,
  ssl: {
    rejectUnauthorized: false  // Only for development; do not use in production without proper SSL setup
  }
});

module.exports = async (req, res) => {
  const newLevel = parseFloat(req.query.level);

  if (newLevel) {
    // Store the new level in the database and respond immediately
    try {
      const client = await pool.connect();

      // Update the new level in the 'valores' table
      await client.query('UPDATE valores SET col2 = $1', [newLevel]);

      res.status(200).json({ level: newLevel });

      client.release();
    } catch (error) {
      console.error('Error during database operations:', error);
      res.status(500).json({ error: "Internal Server Error" });
    }
  } else {
    // If no new level is provided, return the current level from the 'valores' table
    try {
      const client = await pool.connect();
      const result = await client.query('SELECT col2 FROM valores');
      const currentLevel = result.rows.length > 0 ? result.rows[0].col2 : null;
      res.status(200).json({ level: currentLevel });

      client.release();
    } catch (error) {
      console.error('Error during database operations:', error);
      res.status(500).json({ error: "Internal Server Error" });
    }
  }
};
