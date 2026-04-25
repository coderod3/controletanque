import { useEffect, useState } from 'react';
import mqtt from 'mqtt';
// Importação do componente visual que deve estar em components/WaterTank.js
import WaterTank from '../components/WaterTank';

export default function DashboardPage() {
  const [nivel, setNivel] = useState(0);
  const [status, setStatus] = useState('Conectando...');
  const [client, setClient] = useState(null);
  const [volumeInput, setVolumeInput] = useState(10);

  useEffect(() => {
    const host = process.env.NEXT_PUBLIC_MQTT_URL;
    const options = {
      username: 'web_dashboard',
      password: 'Macron@12',
      clientId: 'nexus_web_' + Math.random().toString(16).substring(2, 8),
    };

    const mqttClient = mqtt.connect(host, options);

    mqttClient.on('connect', () => {
      setStatus('Online ✅');
      mqttClient.subscribe('tanque/telemetria');
    });

    mqttClient.on('message', (topic, message) => {
      if (topic === 'tanque/telemetria') {
        const data = JSON.parse(message.toString());
        setNivel(data.nivel || 0);
      }
    });

    mqttClient.on('error', (err) => {
      console.error('Erro MQTT:', err);
      setStatus('Erro na Conexão');
    });

    setClient(mqttClient);
    return () => mqttClient.end();
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

  return (
    <div style={{ padding: '40px', fontFamily: 'sans-serif', backgroundColor: '#f8fafc', minHeight: '100vh' }}>
      <h1>Painel de Controle de Tanques</h1>
      <p>Status: <strong>{status}</strong></p>
      
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