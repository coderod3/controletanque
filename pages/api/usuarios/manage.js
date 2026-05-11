import { db } from '@vercel/postgres';
import mqtt from 'mqtt';

export default async function handler(req, res) {
  const client = await db.connect();
  
  // Conecta ao Broker MQTT diretamente pelo Backend Node.js
  const mqttClient = mqtt.connect(process.env.NEXT_PUBLIC_MQTT_URL, {
    username: process.env.NEXT_PUBLIC_MQTT_USER,
    password: process.env.NEXT_PUBLIC_MQTT_PASS,
    clientId: 'vercel_backend_' + Math.random().toString(16).substring(2, 8),
    protocol: 'wss',
    rejectUnauthorized: false
  });

  try {
    if (req.method === 'POST') {
      // 1. Cadastra na Nuvem (Neon DB)
      const { rfid_uid, nome } = req.body;
      await client.sql`INSERT INTO usuarios (rfid_uid, nome) VALUES (${rfid_uid}, ${nome})`;
      
      // 2. Replica na Borda (ESP32 Flash NVS) automaticamente
      mqttClient.publish('tanque/comando', JSON.stringify({
        comando: "SYNC_USER", uid: rfid_uid, nome: nome, ativo: true
      }));
      
      return res.status(200).json({ success: true, message: 'Usuário criado e injetado na placa!' });
    } 
    
    else if (req.method === 'DELETE') {
      // 1. Deleta da Nuvem
      const { rfid_uid } = req.body;
      await client.sql`DELETE FROM usuarios WHERE rfid_uid = ${rfid_uid}`;
      
      // 2. Revoga na Borda automaticamente (ativo: false)
      mqttClient.publish('tanque/comando', JSON.stringify({
        comando: "SYNC_USER", uid: rfid_uid, nome: "", ativo: false
      }));
      
      return res.status(200).json({ success: true, message: 'Usuário deletado e acesso revogado na placa!' });
    }
    
    return res.status(405).json({ error: 'Method not allowed' });
  } catch (error) {
    console.error("Erro na gestão de usuários:", error);
    return res.status(500).json({ error: error.message });
  } finally {
    client.release();
    mqttClient.end();
  }
}