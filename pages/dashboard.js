import { useEffect, useState } from 'react';
import mqtt from 'mqtt';
import WaterTank from '../components/WaterTank';

export default function DashboardPage() {
  const [nivel, setNivel] = useState(0);
  const [statusMqtt, setStatusMqtt] = useState('Conectando...');
  const [placaOnline, setPlacaOnline] = useState(false); // Consciência do LWT
  const [client, setClient] = useState(null);
  const [volumeInput, setVolumeInput] = useState(10);
  const [operador, setOperador] = useState('Nenhum');
  
  const [statusTanqueDB, setStatusTanqueDB] = useState('CARREGANDO...');
  const [configDB, setConfigDB] = useState({ max: 100, vazio: 100, cheio: 10 });

  const fetchDigitalTwin = async () => {
    try {
      const res = await fetch('/api/tanque/status');
      if (res.ok) {
        const data = await res.json();
        // Preserva o status visual de "OFFLINE" se o LWT do MQTT assim disser
        if (placaOnline || statusTanqueDB === 'CARREGANDO...') {
          setStatusTanqueDB(data.status_operacional);
        }
        setConfigDB({ max: data.volume_maximo, vazio: data.distancia_vazio, cheio: data.distancia_cheio });
      }
    } catch (e) {
      console.error("Erro no Gêmeo Digital:", e);
    }
  };

  useEffect(() => {
    fetchDigitalTwin();
    const interval = setInterval(fetchDigitalTwin, 3000);
    return () => clearInterval(interval);
  }, [placaOnline]); // O hook agora respeita a vida/morte da placa

  useEffect(() => {
    const host = process.env.NEXT_PUBLIC_MQTT_URL;
    if (!host) return;

    const options = {
      username: process.env.NEXT_PUBLIC_MQTT_USER || 'web_dashboard',
      password: process.env.NEXT_PUBLIC_MQTT_PASS || 'Macron@12',
      clientId: 'nexus_web_' + Math.random().toString(16).substring(2, 8),
      protocol: 'wss',
      rejectUnauthorized: false
    };

    const mqttClient = mqtt.connect(host, options);

    mqttClient.on('connect', () => {
      setStatusMqtt('Conectado à Nuvem 🌐');
      mqttClient.subscribe('tanque/telemetria');
      mqttClient.subscribe('tanque/logs'); // FASE 2: Escuta as rejeições da placa
    });

    mqttClient.on('message', (topic, message) => {
      try {
        if (topic === 'tanque/telemetria') {
          const data = JSON.parse(message.toString());
          if (data.nivel !== undefined) setNivel(data.nivel);
          if (data.operador) setOperador(data.operador);
          
          // FASE 2: Captura o Last Will and Testament
          if (data.status_operacional) {
            if (data.status_operacional.includes('OFFLINE')) {
              setPlacaOnline(false);
              setStatusTanqueDB('OFFLINE ❌');
            } else {
              setPlacaOnline(true);
              if (data.status_operacional !== 'ONLINE ✅') {
                setStatusTanqueDB(data.status_operacional);
              }
            }
          }
        } 
        else if (topic === 'tanque/logs') {
          const msg = message.toString();
          // FASE 2: Feedback imersivo via pop-up se a placa recusar o pedido
          if (msg.includes("NEGADO") || msg.includes("ERRO")) {
            alert(`⚠️ AVISO DE HARDWARE:\n\nA placa rejeitou o comando. Motivo: ${msg}`);
          }
        }
      } catch (e) {}
    });

    setClient(mqttClient);
    return () => { if (mqttClient) mqttClient.end(); };
  }, []);

  const enviarComando = (vol, encher) => {
    if (!placaOnline) return alert("Ação bloqueada: A placa está sem energia ou sem rede.");
    if (client?.connected && statusTanqueDB === 'IDLE') {
      const acao = encher ? "ENCHER" : "ESVAZIAR";
      client.publish('tanque/comando', JSON.stringify({
        comando: acao, valor: parseFloat(vol)
      }));
    }
  };

  const dispararCalibracao = () => {
    if (!placaOnline) return alert("Ação bloqueada: A placa está sem energia ou sem rede.");
    if (client?.connected) {
      client.publish('tanque/comando', JSON.stringify({
        comando: "CALIBRAR", max_vol: parseFloat(configDB.max), dist_vazio: parseFloat(configDB.vazio), dist_cheio: parseFloat(configDB.cheio)
      }));
      alert("Comando de calibração enviado à memória Flash do ESP32.");
    }
  };

  // Trava botões e altera opacidade se a placa estiver operando ou morta (Offline)
  const isOcupado = statusTanqueDB !== 'IDLE' || !placaOnline;

  return (
    <div style={{ padding: '40px', fontFamily: 'sans-serif', backgroundColor: '#f8fafc', minHeight: '100vh' }}>
      <h1>Painel de Controle e Gêmeo Digital</h1>
      
      <div style={{ display: 'flex', gap: '20px', marginBottom: '20px', flexWrap: 'wrap' }}>
        <div style={{ padding: '15px', backgroundColor: 'white', borderRadius: '8px', border: '1px solid #e2e8f0', flex: '1 1 250px' }}>
          <p style={{ margin: 0, color: '#64748b', fontSize: '0.8rem' }}>MQTT (SERVER NUvem)</p>
          <strong style={{ fontSize: '1.2rem' }}>{statusMqtt}</strong>
        </div>
        <div style={{ padding: '15px', backgroundColor: 'white', borderRadius: '8px', border: '1px solid #e2e8f0', flex: '1 1 250px' }}>
          <p style={{ margin: 0, color: '#64748b', fontSize: '0.8rem' }}>STATUS FÍSICO DO ESP32</p>
          <strong style={{ fontSize: '1.2rem', color: !placaOnline ? '#ef4444' : isOcupado ? '#f59e0b' : '#22c55e' }}>
            {statusTanqueDB}
          </strong>
        </div>
        <div style={{ padding: '15px', backgroundColor: 'white', borderRadius: '8px', border: '1px solid #e2e8f0', flex: '1 1 250px' }}>
          <p style={{ margin: 0, color: '#64748b', fontSize: '0.8rem' }}>OPERADOR LOCAL</p>
          <strong style={{ fontSize: '1.2rem', color: '#2563eb' }}>{operador}</strong>
        </div>
      </div>
      
      <div style={{ display: 'flex', gap: '40px', marginTop: '30px', alignItems: 'flex-start', flexWrap: 'wrap' }}>
        <div style={{ flex: '1 1 300px', display: 'flex', justifyContent: 'center' }}>
          <WaterTank nivel={nivel} />
        </div>
        
        <div style={{ flex: '1 1 400px' }}>
          <div style={{ backgroundColor: 'white', padding: '20px', borderRadius: '12px', border: '1px solid #e2e8f0', marginBottom: '20px' }}>
            <h3 style={{ marginTop: 0 }}>Operação de Volume</h3>
            <label style={{ display: 'block', marginBottom: '10px' }}>Volume (Litros):</label>
            <input 
              type="number" value={volumeInput} onChange={(e) => setVolumeInput(e.target.value)}
              disabled={isOcupado}
              style={{ fontSize: '20px', padding: '10px', width: '100px', marginBottom: '20px', opacity: isOcupado ? 0.5 : 1 }}
            />
            <div style={{ display: 'flex', gap: '10px' }}>
              <button 
                onClick={() => enviarComando(volumeInput, true)} disabled={isOcupado}
                style={{ padding: '15px', backgroundColor: isOcupado ? '#94a3b8' : '#2563eb', color: 'white', border: 'none', borderRadius: '8px', cursor: isOcupado ? 'not-allowed' : 'pointer', flex: 1 }}
              >
                ENCHER
              </button>
              <button 
                onClick={() => enviarComando(volumeInput, false)} disabled={isOcupado}
                style={{ padding: '15px', backgroundColor: isOcupado ? '#94a3b8' : '#1e293b', color: 'white', border: 'none', borderRadius: '8px', cursor: isOcupado ? 'not-allowed' : 'pointer', flex: 1 }}
              >
                ESVAZIAR
              </button>
            </div>
            
            <div style={{ display: 'flex', gap: '10px', marginTop: '20px' }}>
              <button 
                onClick={() => client.publish('tanque/comando', JSON.stringify({ comando: "PARAR" }))}
                disabled={!placaOnline}
                style={{ flex: 2, padding: '15px', backgroundColor: !placaOnline ? '#fca5a5' : '#ef4444', color: 'white', border: 'none', borderRadius: '8px', fontWeight: 'bold', cursor: !placaOnline ? 'not-allowed' : 'pointer' }}
              >
                PARADA DE EMERGÊNCIA
              </button>
              <button 
                onClick={() => client.publish('tanque/comando', JSON.stringify({ comando: "RESET" }))}
                disabled={!placaOnline}
                style={{ flex: 1, padding: '15px', backgroundColor: !placaOnline ? '#fcd34d' : '#f59e0b', color: 'white', border: 'none', borderRadius: '8px', fontWeight: 'bold', cursor: !placaOnline ? 'not-allowed' : 'pointer' }}
              >
                RESETAR FALHA
              </button>
            </div>
          </div>

          <div style={{ backgroundColor: '#f1f5f9', padding: '20px', borderRadius: '12px', border: '1px solid #cbd5e1', marginBottom: '20px' }}>
            <h3 style={{ marginTop: 0, color: '#334155' }}>Diagnóstico de Hardware (Bypass)</h3>
            <p style={{ fontSize: '0.85rem', color: '#475569' }}>Testa conexões elétricas (Pulso de 2s).</p>
            <div style={{ display: 'flex', gap: '10px' }}>
              <button 
                onClick={() => client.publish('tanque/comando', JSON.stringify({ comando: "TESTE_BOMBA", encher: true }))}
                disabled={!placaOnline}
                style={{ flex: 1, padding: '10px', backgroundColor: !placaOnline ? '#94a3b8' : '#334155', color: 'white', border: 'none', borderRadius: '6px', cursor: !placaOnline ? 'not-allowed' : 'pointer' }}
              >
                TESTE BOMBA 1
              </button>
              <button 
                onClick={() => client.publish('tanque/comando', JSON.stringify({ comando: "TESTE_BOMBA", encher: false }))}
                disabled={!placaOnline}
                style={{ flex: 1, padding: '10px', backgroundColor: !placaOnline ? '#94a3b8' : '#334155', color: 'white', border: 'none', borderRadius: '6px', cursor: !placaOnline ? 'not-allowed' : 'pointer' }}
              >
                TESTE BOMBA 2
              </button>
            </div>
          </div>

          <div style={{ backgroundColor: '#fffbeb', padding: '20px', borderRadius: '12px', border: '1px solid #fde68a' }}>
            <h3 style={{ marginTop: 0, color: '#b45309' }}>Ajuste de Calibração (Memória Flash)</h3>
            <div style={{ display: 'flex', gap: '10px', marginBottom: '15px' }}>
              <div style={{ flex: 1 }}>
                <label style={{ fontSize: '0.8rem', color: '#92400e' }}>Vol. Max (L)</label>
                <input type="number" value={configDB.max} onChange={(e) => setConfigDB({...configDB, max: e.target.value})} style={{ width: '100%', padding: '8px' }} />
              </div>
              <div style={{ flex: 1 }}>
                <label style={{ fontSize: '0.8rem', color: '#92400e' }}>Vazio (cm)</label>
                <input type="number" value={configDB.vazio} onChange={(e) => setConfigDB({...configDB, vazio: e.target.value})} style={{ width: '100%', padding: '8px' }} />
              </div>
              <div style={{ flex: 1 }}>
                <label style={{ fontSize: '0.8rem', color: '#92400e' }}>Cheio (cm)</label>
                <input type="number" value={configDB.cheio} onChange={(e) => setConfigDB({...configDB, cheio: e.target.value})} style={{ width: '100%', padding: '8px' }} />
              </div>
            </div>
            <button 
              onClick={dispararCalibracao} disabled={!placaOnline}
              style={{ width: '100%', padding: '10px', backgroundColor: !placaOnline ? '#fcd34d' : '#b45309', color: 'white', border: 'none', borderRadius: '6px', cursor: !placaOnline ? 'not-allowed' : 'pointer' }}
            >
              SINCRONIZAR CALIBRAÇÃO
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}