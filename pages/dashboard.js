import { useEffect, useState } from 'react';
import mqtt from 'mqtt';
import WaterTank from '../components/WaterTank'; // Ajuste o caminho se necessário

export default function DashboardPage() {
  const [nivel, setNivel] = useState(0);
  const [statusMqtt, setStatusMqtt] = useState('Conectando...');
  const [client, setClient] = useState(null);
  const [volumeInput, setVolumeInput] = useState(10);
  const [operador, setOperador] = useState('Nenhum');
  
  // ESTADOS PARA O GÊMEO DIGITAL
  const [statusTanqueDB, setStatusTanqueDB] = useState('CARREGANDO...');
  const [configDB, setConfigDB] = useState({ max: 100, vazio: 100, cheio: 10 });

  // FUNÇÃO PARA LER O BANCO DE DADOS (Rota Singular corrigida)
  const fetchDigitalTwin = async () => {
    try {
      const res = await fetch('/api/tanque/status');
      if (res.ok) {
        const data = await res.json();
        setStatusTanqueDB(data.status_operacional);
        setConfigDB({
          max: data.volume_maximo,
          vazio: data.distancia_vazio,
          cheio: data.distancia_cheio
        });
      }
    } catch (e) {
      console.error("Erro ao sincronizar Gêmeo Digital:", e);
    }
  };

  // LOOP DE SINCRONIZAÇÃO (a cada 3 segundos)
  useEffect(() => {
    fetchDigitalTwin();
    const interval = setInterval(fetchDigitalTwin, 3000);
    return () => clearInterval(interval);
  }, []);

  // CONFIGURAÇÃO MQTT
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
      setStatusMqtt('Online ✅');
      mqttClient.subscribe('tanque/telemetria');
    });

    mqttClient.on('message', (topic, message) => {
      if (topic === 'tanque/telemetria') {
        try {
          const data = JSON.parse(message.toString());
          setNivel(data.nivel || 0);
          setOperador(data.operador || 'Nenhum');
        } catch (e) {}
      }
    });

    setClient(mqttClient);
    return () => { if (mqttClient) mqttClient.end(); };
  }, []);

  // Ações de Comando Remoto (Alinhado com TankController.cpp)
  const enviarComando = (vol, encher) => {
    if (client?.connected && statusTanqueDB === 'IDLE') {
      const acao = encher ? "ENCHER" : "ESVAZIAR";
      client.publish('tanque/comando', JSON.stringify({
        comando: acao, 
        valor: parseFloat(vol)
      }));
    }
  };

  // Ação de Calibração Remota (Alinhado com TankPhysics.cpp)
  const dispararCalibracao = () => {
    if (client?.connected) {
      client.publish('tanque/comando', JSON.stringify({
        comando: "CALIBRAR",
        max_vol: parseFloat(configDB.max),
        dist_vazio: parseFloat(configDB.vazio),
        dist_cheio: parseFloat(configDB.cheio)
      }));
      alert("Comando de calibração enviado! O ESP32 salvará isso na Flash.");
    }
  };

  const isOcupado = statusTanqueDB !== 'IDLE';

  return (
    <div style={{ padding: '40px', fontFamily: 'sans-serif', backgroundColor: '#f8fafc', minHeight: '100vh' }}>
      <h1>Painel de Controle e Gêmeo Digital</h1>
      
      {/* GRID DE STATUS RESPONSIVO */}
      <div style={{ display: 'flex', gap: '20px', marginBottom: '20px', flexWrap: 'wrap' }}>
        <div style={{ padding: '15px', backgroundColor: 'white', borderRadius: '8px', border: '1px solid #e2e8f0', flex: '1 1 250px' }}>
          <p style={{ margin: 0, color: '#64748b', fontSize: '0.8rem' }}>CONEXÃO MQTT (TEMPO REAL)</p>
          <strong style={{ fontSize: '1.2rem' }}>{statusMqtt}</strong>
        </div>
        <div style={{ padding: '15px', backgroundColor: 'white', borderRadius: '8px', border: '1px solid #e2e8f0', flex: '1 1 250px' }}>
          <p style={{ margin: 0, color: '#64748b', fontSize: '0.8rem' }}>STATUS DO HARDWARE (BANCO DE DADOS)</p>
          <strong style={{ fontSize: '1.2rem', color: isOcupado ? '#ef4444' : '#22c55e' }}>{statusTanqueDB}</strong>
        </div>
        <div style={{ padding: '15px', backgroundColor: 'white', borderRadius: '8px', border: '1px solid #e2e8f0', flex: '1 1 250px' }}>
          <p style={{ margin: 0, color: '#64748b', fontSize: '0.8rem' }}>OPERADOR LOCAL</p>
          <strong style={{ fontSize: '1.2rem', color: '#2563eb' }}>{operador}</strong>
        </div>
      </div>
      
      <div style={{ display: 'flex', gap: '40px', marginTop: '30px', alignItems: 'flex-start', flexWrap: 'wrap' }}>        <WaterTank nivel={nivel} />
        
        <div style={{ flex: 1 }}>
          {/* PAINEL DE CONTROLE DE TAREFAS */}
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
                onClick={() => enviarComando(volumeInput, true)} 
                disabled={isOcupado}
                style={{ padding: '15px', backgroundColor: isOcupado ? '#94a3b8' : '#2563eb', color: 'white', border: 'none', borderRadius: '8px', cursor: isOcupado ? 'not-allowed' : 'pointer', flex: 1 }}
              >
                ENCHER
              </button>
              <button 
                onClick={() => enviarComando(volumeInput, false)} 
                disabled={isOcupado}
                style={{ padding: '15px', backgroundColor: isOcupado ? '#94a3b8' : '#1e293b', color: 'white', border: 'none', borderRadius: '8px', cursor: isOcupado ? 'not-allowed' : 'pointer', flex: 1 }}
              >
                ESVAZIAR
              </button>
            </div>
            
            {/* BOTÕES DE EMERGÊNCIA E RESET */}
            <div style={{ display: 'flex', gap: '10px', marginTop: '20px' }}>
              <button 
                onClick={() => client.publish('tanque/comando', JSON.stringify({ comando: "PARAR" }))}
                style={{ flex: 2, padding: '15px', backgroundColor: '#ef4444', color: 'white', border: 'none', borderRadius: '8px', fontWeight: 'bold', cursor: 'pointer' }}
              >
                PARADA DE EMERGÊNCIA
              </button>
              <button 
                onClick={() => client.publish('tanque/comando', JSON.stringify({ comando: "RESET" }))}
                style={{ flex: 1, padding: '15px', backgroundColor: '#f59e0b', color: 'white', border: 'none', borderRadius: '8px', fontWeight: 'bold', cursor: 'pointer' }}
              >
                RESETAR FALHA
              </button>
            </div>
          </div>

          {/* PAINEL DE DIAGNÓSTICO E HARDWARE */}
          <div style={{ backgroundColor: '#f1f5f9', padding: '20px', borderRadius: '12px', border: '1px solid #cbd5e1', marginBottom: '20px' }}>
            <h3 style={{ marginTop: 0, color: '#334155' }}>Diagnóstico de Hardware (Bypass)</h3>
            <p style={{ fontSize: '0.85rem', color: '#475569' }}>Testa as conexões físicas ignorando a máquina de estados (Pulso de 2s).</p>
            <div style={{ display: 'flex', gap: '10px' }}>
              <button 
                onClick={() => client.publish('tanque/comando', JSON.stringify({ comando: "TESTE_BOMBA", encher: true }))}
                style={{ flex: 1, padding: '10px', backgroundColor: '#334155', color: 'white', border: 'none', borderRadius: '6px', cursor: 'pointer' }}
              >
                TESTE BOMBA 1 (ENCHER)
              </button>
              <button 
                onClick={() => client.publish('tanque/comando', JSON.stringify({ comando: "TESTE_BOMBA", encher: false }))}
                style={{ flex: 1, padding: '10px', backgroundColor: '#334155', color: 'white', border: 'none', borderRadius: '6px', cursor: 'pointer' }}
              >
                TESTE BOMBA 2 (ESVAZIAR)
              </button>
            </div>
          </div>
            
            <button 
              onClick={() => client.publish('tanque/comando', JSON.stringify({ comando: "PARAR" }))}
              style={{ marginTop: '20px', width: '100%', padding: '15px', backgroundColor: '#ef4444', color: 'white', border: 'none', borderRadius: '8px', fontWeight: 'bold', cursor: 'pointer' }}
            >
              PARADA DE EMERGÊNCIA (SOBRESCREVE TRAVA)
            </button>
          </div>

          {/* PAINEL DE ENGENHARIA / CALIBRAÇÃO */}
          <div style={{ backgroundColor: '#fffbeb', padding: '20px', borderRadius: '12px', border: '1px solid #fde68a' }}>
            <h3 style={{ marginTop: 0, color: '#b45309' }}>Ajuste de Calibração (Hardware Flash)</h3>
            <p style={{ fontSize: '0.85rem', color: '#78350f' }}>Altere os parâmetros físicos e sincronize com a memória do ESP32.</p>
            
            <div style={{ display: 'flex', gap: '10px', marginBottom: '15px' }}>
              <div style={{ flex: 1 }}>
                <label style={{ fontSize: '0.8rem', color: '#92400e' }}>Vol. Máximo (L)</label>
                <input type="number" value={configDB.max} onChange={(e) => setConfigDB({...configDB, max: e.target.value})} style={{ width: '100%', padding: '8px' }} />
              </div>
              <div style={{ flex: 1 }}>
                <label style={{ fontSize: '0.8rem', color: '#92400e' }}>Dist. Vazio (cm)</label>
                <input type="number" value={configDB.vazio} onChange={(e) => setConfigDB({...configDB, vazio: e.target.value})} style={{ width: '100%', padding: '8px' }} />
              </div>
              <div style={{ flex: 1 }}>
                <label style={{ fontSize: '0.8rem', color: '#92400e' }}>Dist. Cheio (cm)</label>
                <input type="number" value={configDB.cheio} onChange={(e) => setConfigDB({...configDB, cheio: e.target.value})} style={{ width: '100%', padding: '8px' }} />
              </div>
            </div>
            
            <button 
              onClick={dispararCalibracao}
              style={{ width: '100%', padding: '10px', backgroundColor: '#b45309', color: 'white', border: 'none', borderRadius: '6px', cursor: 'pointer' }}
            >
              SINCRONIZAR CALIBRAÇÃO (SYNC_CONFIG)
            </button>
          </div>

        </div>
      </div>
    </div>
  );
}