import pool from '../../lib/db';

export default async function handler(req, res) {
  if (req.method !== 'POST') {
    return res.status(405).json({ message: 'Método não permitido.' });
  }

  const { matricula, password } = req.body;

  try {
    const result = await pool.query(
      'SELECT * FROM usuarios WHERE matricula = $1',
      [matricula] 
    );

    if (result.rows.length === 0) {
      return res.status(401).json({ message: 'Matrícula não encontrada.' });
    }

    const user = result.rows[0];
    
    // Suporta tanto a coluna "senha" quanto "password" caso haja variação no seu banco
    const dbPassword = user.senha || user.password;

    if (password !== dbPassword) {
      return res.status(401).json({ message: 'Chave de segurança incorreta.' });
    }

    let userRole = 'visualizacao'; 

    // Adaptado para suportar os valores que você usa no seu formulário (Gestão/Produção/Manutenção) 
    // ou os valores em minúsculo do seu exemplo.
    if (user.setor === 'gestor' || user.setor === 'Gestão') {
      userRole = 'gestor';
    } else if (user.setor === 'operador' || user.setor === 'Produção') {
      userRole = 'operador';
    } else if (user.setor === 'visualizador' || user.setor === 'Manutenção') {
      userRole = 'visualizacao'; 
    }

    return res.status(200).json({ 
      message: 'Autenticado com sucesso',
      role: userRole,
      user: { nome: user.nome, setor: user.setor }
    });

  } catch (error) {
    console.error('Erro na autenticação:', error);
    return res.status(500).json({ message: 'Erro interno do servidor.' });
  }
}