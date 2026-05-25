import { sql } from '@vercel/postgres';

export default async function handler(req, res) {
  if (req.method === 'POST') {
    const { matricula, nome, email, setor, rfid_id, limite_encher, limite_esvaziar } = req.body;
    const tag = rfid_id.trim() === '' ? null : rfid_id.toUpperCase();

    try {
      await sql`
        INSERT INTO usuarios (matricula, nome, email, setor, rfid_id, limite_encher, limite_esvaziar)
        VALUES (${matricula}, ${nome}, ${email}, ${setor}, ${tag}, ${limite_encher}, ${limite_esvaziar})
      `;
      return res.status(200).json({ message: 'Usuário cadastrado com sucesso.' });
    } catch (error) {
      console.error(error);
      return res.status(500).json({ error: 'Erro ao cadastrar usuário.' });
    }
  } 
  
  // NOVA ROTA: Editar Usuário Existente
  else if (req.method === 'PUT') {
    const { id, nome, email, setor, rfid_id, limite_encher, limite_esvaziar } = req.body;
    const tag = rfid_id.trim() === '' ? null : rfid_id.toUpperCase();

    try {
      // Não permitimos editar a matrícula (login), apenas os dados e permissões
      await sql`
        UPDATE usuarios 
        SET nome = ${nome}, email = ${email}, setor = ${setor}, rfid_id = ${tag}, limite_encher = ${limite_encher}, limite_esvaziar = ${limite_esvaziar}
        WHERE id = ${id}
      `;
      return res.status(200).json({ message: 'Usuário atualizado com sucesso.' });
    } catch (error) {
      console.error(error);
      return res.status(500).json({ error: 'Erro ao atualizar usuário.' });
    }
  }

  else if (req.method === 'DELETE') {
    const { id } = req.body;
    try {
      await sql`DELETE FROM usuarios WHERE id = ${id}`;
      return res.status(200).json({ message: 'Usuário deletado.' });
    } catch (error) {
      return res.status(500).json({ error: 'Erro ao deletar.' });
    }
  }

  res.setHeader('Allow', ['POST', 'PUT', 'DELETE']);
  res.status(405).end(`Method ${req.method} Not Allowed`);
}