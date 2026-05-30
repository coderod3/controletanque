import Head from 'next/head';
import { initMqtt } from '../lib/mqttService'; 
import '../styles/globals.css'; 

// Executa uma única vez no carregamento inicial do navegador
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