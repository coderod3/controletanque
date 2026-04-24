import { Pool } from 'pg';

// Cria a pool de conexão usando a variável de ambiente que você já tem
const pool = new Pool({
  connectionString: process.env.POSTGRES_URL,
  ssl: { rejectUnauthorized: false }
});

export default pool;