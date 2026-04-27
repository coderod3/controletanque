import { useEffect, useState } from 'react';
import mqtt from 'mqtt';
// Importação do componente visual que deve estar em components/WaterTank.js
import WaterTank from '../components/WaterTank';

export default function DashboardPage() {
  const [nivel, setNivel] = useState(0);
  const [status, setStatus] = useState('Conectando...');
  const [client, setClient] = useState(null);
  const [volumeInput, setVolumeInput] = useState(10);
  const [operador, setOperador] = useState('Nenhum');

  useEffect(() => {
    const host = process.env.NEXT_PUBLIC_MQTT_URL;
    
    // Debug: Isso vai aparecer no console do seu navegador (F12)
    console.log("Configuração MQTT - Host:", host);
    console.log("Configuração MQTT - User:", process.env.NEXT_PUBLIC_MQTT_USER);

    if (!host) {
      setStatus('Erro: Variáveis de Ambiente ausentes na Vercel ❌');
      return;
    }

    const options = {
      username: process.env.NEXT_PUBLIC_MQTT_USER || 'web_dashboard',
      password: process.env.NEXT_PUBLIC_MQTT_PASS || 'Macron@12',
      clientId: 'nexus_web_' + Math.random().toString(16).substring(2, 8),
      // Força o uso de WebSocket Seguro para evitar o erro de Mixed Content
      protocol: 'wss',
      rejectUnauthorized: false // Útil se houver problemas de certificado no broker
    };

    const mqttClient = mqtt.connect(host, options);

    mqttClient.on('connect', () => {
      console.log("Conectado com sucesso ao Broker!");
      setStatus('Online ✅');
      mqttClient.subscribe('tanque/telemetria');
    });

    mqttClient.on('message', (topic, message) => {
      if (topic === 'tanque/telemetria') {
        try {
          const data = JSON.parse(message.toString());
          setNivel(data.nivel || 0);
          setOperador(data.operador || 'Nenhum'); // PEGA O NOME DO JSON
        } catch (e) {
          console.error("Erro no parse do JSON:", e);
        }
      }
    });

    mqttClient.on('error', (err) => {
      console.error('Erro de conexão MQTT:', err);
      setStatus('Erro na Conexão');
    });

    setClient(mqttClient);
    return () => {
      if (mqttClient) mqttClient.end();
    };
  }, []);

  const enviarComando = (vol, encher) => {
    if (client?.connected) {
      client.publish('tanque/comando', JSON.stringify({
        acao: "EXECUTAR",
        volume: parseFloat(vol),
        encher: encher
      }));
    }
  };
  
  const dispararCalibracao = (novoMax, novaVazio, novaCheio) => {
    if (client?.connected) {
      const payload = {
        acao: "SYNC_CONFIG",
        max_volume: parseFloat(novoMax),
        dist_vazio: parseFloat(novaVazio),
        dist_cheio: parseFloat(novaCheio)
      };
      
      client.publish('tanque/comando', JSON.stringify(payload));
      console.log("Comando de calibração enviado:", payload);
    }
  };

  return (
    <div style={{ padding: '40px', fontFamily: 'sans-serif', backgroundColor: '#f8fafc', minHeight: '100vh' }}>
      <h1>Painel de Controle de Tanques</h1>
      <p>Status: <strong>{status}</strong></p>
      <p>Operador Ativo: <strong style={{ color: '#2563eb' }}>{operador}</strong></p>
      
      <div style={{ display: 'flex', gap: '50px', marginTop: '30px', alignItems: 'center' }}>
        <WaterTank nivel={nivel} />
        
        <div style={{ backgroundColor: 'white', padding: '20px', borderRadius: '12px', border: '1px solid #e2e8f0' }}>
          <label style={{ display: 'block', marginBottom: '10px' }}>Volume (Litros):</label>
          <input 
            type="number" 
            value={volumeInput} 
            onChange={(e) => setVolumeInput(e.target.value)}
            style={{ fontSize: '20px', padding: '10px', width: '100px', marginBottom: '20px' }}
          />
          <div style={{ display: 'flex', gap: '10px' }}>
            <button onClick={() => enviarComando(volumeInput, true)} style={{ padding: '15px', backgroundColor: '#2563eb', color: 'white', border: 'none', borderRadius: '8px', cursor: 'pointer' }}>ENCHER</button>
            <button onClick={() => enviarComando(volumeInput, false)} style={{ padding: '15px', backgroundColor: '#1e293b', color: 'white', border: 'none', borderRadius: '8px', cursor: 'pointer' }}>ESVAZIAR</button>
          </div>
          <button 
            onClick={() => client.publish('tanque/comando', JSON.stringify({ acao: "PARAR" }))}
            style={{ marginTop: '20px', width: '100%', padding: '15px', backgroundColor: '#ef4444', color: 'white', border: 'none', borderRadius: '8px', fontWeight: 'bold', cursor: 'pointer' }}
          >
            PARAR TUDO
          </button>
        </div>
      </div>
    </div>
  );
}