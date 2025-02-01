require('dotenv').config({ path: '.env.development.local' });
const { Pool } = require('pg');
const pool = new Pool({
    connectionString: process.env.POSTGRES_URL,
    ssl: { rejectUnauthorized: false }
});

export default async function handler(req, res) {
    const pin = "lampada";
    const stateParam = req.query.state;

    try {
        const client = await pool.connect();

        if (stateParam !== undefined) {
            // Convert parameter to boolean: "ON" (or "on") is true, anything else is false.
            const state = stateParam.toLowerCase() === "on";
            const upsertQuery = `
        INSERT INTO pin_states (pin, state)
        VALUES ($1, $2)
        ON CONFLICT (pin) DO UPDATE 
        SET state = EXCLUDED.state, updated_at = current_timestamp
      `;
            await client.query(upsertQuery, [pin, state]);
            res.status(200).json({ pin, state });
        } else {
            const selectQuery = 'SELECT state FROM pin_states WHERE pin = $1';
            const result = await client.query(selectQuery, [pin]);
            if (result.rows.length > 0) {
                res.status(200).json({ pin, state: result.rows[0].state });
            } else {
                // If no entry exists yet, default to false.
                res.status(200).json({ pin, state: false });
            }
        }
        client.release();
    } catch (error) {
        console.error('Error in lampada endpoint:', error);
        res.status(500).json({ error: "Internal Server Error" });
    }
}
