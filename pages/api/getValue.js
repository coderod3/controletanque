import { sql } from "@vercel/postgres";

export default async function handler(req, res) {
  try {
    const { rows } = await sql`SELECT col1 FROM VALORES LIMIT 1`;

    if (rows.length > 0) {
      const value = rows[0].col1;
      res.status(200).json({ value });
    } else {
      res.status(404).json({ error: "No data found" });
    }
  } catch (error) {
    res.status(500).json({ error: "Internal Server Error" });
  }
}
