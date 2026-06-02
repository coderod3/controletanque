import pool from '../../lib/db';

export default async function handler(req, res) {
  if (req.method !== 'POST') {
    return res.status(405).json({ message: 'Método não permitido.' });
  }

  // O .trim() remove espaços vazios acidentais no início e fim do texto
  const matricula = (req.body.matricula || '').trim();
  const password = req.body.password;

  console.log(`[AUTH API] Tentativa de login para matrícula: "${matricula}"`);

  try {
    const result = await pool.query(
      'SELECT * FROM usuarios WHERE matricula = $1',
      [matricula] 
    );

    if (result.rows.length === 0) {
      console.log(`[AUTH API] Matrícula "${matricula}" não encontrada no banco.`);
      return res.status(401).json({ message: 'Matrícula não encontrada.' });
    }

    const user = result.rows[0];
    const dbPassword = user.senha || user.password;

    console.log(`[AUTH API] Matrícula encontrada. Setor do usuário: "${user.setor}"`);

    if (password !== dbPassword) {
      console.log(`[AUTH API] Falha: A senha digitada não corresponde à do banco.`);
      return res.status(401).json({ message: 'Chave de segurança incorreta.' });
    }

    let userRole = 'visualizacao'; 

    if (user.setor === 'gestor' || user.setor === 'Gestão') {
      userRole = 'gestor';
    } else if (user.setor === 'operador' || user.setor === 'Produção') {
      userRole = 'operador';
    } else if (user.setor === 'visualizador' || user.setor === 'Manutenção') {
      userRole = 'visualizacao'; 
    }

    console.log(`[AUTH API] Sucesso. Cargo definido: ${userRole}`);
    
    return res.status(200).json({ 
      message: 'Autenticado com sucesso',
      role: userRole,
      user: { 
        nome: user.nome, 
        setor: user.setor,
        matricula: user.matricula,
        // Envia os limites do banco para o navegador
        limite_encher: parseFloat(user.limite_encher) || 0,
        limite_esvaziar: parseFloat(user.limite_esvaziar) || 0
      }
    });
    
  } catch (error) {
    console.error('[AUTH API] Erro fatal no servidor:', error);
    return res.status(500).json({ message: 'Erro interno do servidor.' });
  }
}