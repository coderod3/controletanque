import Head from 'next/head';
import '../styles/globals.css'; 
import { initMqtt } from '../lib/mqttService'; // Importe a função aqui

// ISSO RODA UMA ÚNICA VEZ QUANDO O CÓDIGO JS É CARREGADO NO NAVEGADOR
// E não entra nos loops de renderização do React!
if (typeof window !== 'undefined') {
  initMqtt();
}

export default function App({ Component, pageProps }) {
  return (
    <>
      <Head>
        <link rel="icon" href="/logo.png" />
        <meta name="viewport" content="width=device-width, initial-scale=1" />
      </Head>
      <Component {...pageProps} />
    </>
  );
}