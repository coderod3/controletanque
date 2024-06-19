import { sql } from "@vercel/postgres";

export default async function handler(req, res) {
  try {
    // Consulta o banco de dados
    const { rows } = await sql`SELECT col1 FROM valores LIMIT 1`;

    // Verifica se há resultados
    if (rows.length > 0) {
      // Pega o valor da primeira linha
      const value = rows[0].col1;
      
      // Retorna o valor como resposta
      res.status(200).json({ value });
    } else {
      res.status(404).json({ error: "No data found" });
    }
  } catch (error) {
    // Trata possíveis erros
    res.status(500).json({ error: "Internal Server Error" });
  }
}
