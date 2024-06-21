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
  const newLevel = parseFloat(req.query.newLevel);
  const userAgent = req.query.userAgent;
  const userIP = req.query.userIP;

  if (newLevel && userAgent && userIP) {
    // Send the response immediately
    res.status(200).json({ level: newLevel });

    // Log and database operations
    try {
      const client = await pool.connect();

      // Retrieve the previous level from the 'valores' table
      const previousResult = await client.query('SELECT col2 FROM valores');
      const previousLevel = previousResult.rows.length > 0 ? previousResult.rows[0].col2 : null;

      // Update the new level in the 'valores' table
      await client.query('UPDATE valores SET col2 = $1', [newLevel]);

      // Log the information
      const logEntry = {
        text: 'INSERT INTO logs (user_ip, user_agent, previous_level, new_level) VALUES ($1, $2, $3, $4)',
        values: [userIP, userAgent, previousLevel, newLevel]
      };
      await client.query(logEntry);

      client.release();
    } catch (error) {
      console.error('Error during database operations:', error);
    }
  } else {
    try {
      const client = await pool.connect();
      // If no parameters are provided, return the current level from the 'valores' table
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
