require('dotenv').config({ path: '.env.development.local' });
const { Pool } = require('pg');

const pool = new Pool({
    connectionString: process.env.POSTGRES_URL,
    ssl: { rejectUnauthorized: false } // Only for development; adjust for production accordingly.
});

// Allowed pin names
const validPins = new Set(["lampada", "led", "tomada", "outro"]);

export default async function handler(req, res) {
    const { pin, state } = req.query;

    // Validate pin parameter
    if (!pin || !validPins.has(pin)) {
        res.status(400).json({ error: "Invalid or missing pin parameter. Valid options: lampada, led, tomada, outro." });
        return;
    }

    try {
        const client = await pool.connect();

        if (state !== undefined) {
            // If state is provided, update the state.
            // Interpret "ON" (case-insensitive) as ON, and any other value as OFF.
            const stateStr = state.toUpperCase() === "ON" ? "ON" : "OFF";
            const stateBool = stateStr === "ON";

            // Use an UPSERT query to insert or update the state for this pin.
            const upsertQuery = `
        INSERT INTO pin_states (pin, state)
        VALUES ($1, $2)
        ON CONFLICT (pin) DO UPDATE 
        SET state = EXCLUDED.state, updated_at = current_timestamp
      `;
            await client.query(upsertQuery, [pin, stateBool]);
            client.release();
            res.status(200).json({ pin, state: stateStr });
        } else {
            // If no state is provided, retrieve the current state.
            const selectQuery = 'SELECT state FROM pin_states WHERE pin = $1';
            const result = await client.query(selectQuery, [pin]);
            client.release();

            let stateStr = "OFF"; // default if no record exists.
            if (result.rows.length > 0) {
                stateStr = result.rows[0].state ? "ON" : "OFF";
            }
            res.status(200).json({ pin, state: stateStr });
        }
    } catch (error) {
        console.error("Error in pin endpoint:", error);
        res.status(500).json({ error: "Internal Server Error" });
    }
}
