import React from 'react';

export default function WaterTank({ nivel }) {
  const heightPercent = Math.min(Math.max(nivel, 0), 100);

  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center' }}>
      <div style={{ 
        position: 'relative', 
        width: '160px', 
        height: '320px', 
        border: '4px solid #334155', 
        borderRadius: '0 0 16px 16px', 
        backgroundColor: '#f1f5f9', 
        overflow: 'hidden',
        boxShadow: 'inset 0 2px 4px 0 rgb(0 0 0 / 0.05)'
      }}>
        {/* Água */}
        <div style={{ 
          position: 'absolute', 
          bottom: 0, 
          width: '100%', 
          backgroundColor: '#3b82f6', 
          transition: 'height 1s ease-in-out',
          height: `${heightPercent}%` 
        }}>
          <div style={{ width: '100%', height: '8px', backgroundColor: '#60a5fa', opacity: 0.5 }} />
        </div>

        {/* Texto do Nível */}
        <div style={{ 
          position: 'absolute', 
          inset: 0, 
          display: 'flex', 
          alignItems: 'center', 
          justifyContent: 'center',
          pointerEvents: 'none'
        }}>
          <span style={{ 
            fontSize: '1.5rem', 
            fontWeight: 900, 
            color: '#1e293b', 
            backgroundColor: 'rgba(255,255,255,0.5)', 
            padding: '4px 8px', 
            borderRadius: '4px' 
          }}>
            {nivel.toFixed(1)}L
          </span>
        </div>
      </div>
      <p style={{ marginTop: '16px', fontWeight: 'bold', color: '#475569' }}>NÍVEL ATUAL</p>
    </div>
  );
}