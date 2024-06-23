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
  const userAgent = req.query.userAgent;
  const userIP = req.query.userIP;
  const newLevel = parseFloat(req.query.level);

  if (userAgent && userIP && newLevel) {
    try {
      const client = await pool.connect();
      console.log("just befoooooore");
      // Retrieve the previous level from the 'valores' table
      const previousResult = await client.query('SELECT new_level FROM logs ORDER BY id DESC LIMIT 1');
      // const previousLevel = previousResult.rows.length > 0 ? previousResult.rows[0].col2 : null; // here lastvalue  
      console.log(previousResult);
      // Log the information
      const logEntry = {
        text: 'INSERT INTO logs (user_ip, user_agent, previous_level, new_level) VALUES ($1, $2, $3, $4)',
        values: [userIP, userAgent, previousResult, newLevel]
      };
      await client.query(logEntry);

      client.release();
      res.status(200).json({ message: "Log entry created successfully" });
    } catch (error) {
      console.error('Error during database operations:', error);
      res.status(500).json({ error: "Internal Server Error" });
    }
  } else {
    res.status(400).json({ error: "Missing required parameters" });
  }
};
