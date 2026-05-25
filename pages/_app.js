import Head from 'next/head';
// Mantenha a importação do seu CSS global aqui (geralmente '../styles/globals.css')
import '../styles/globals.css'; 

export default function App({ Component, pageProps }) {
  return (
    <>
      <Head>
        {/* Define o ícone da aba do navegador para todo o sistema */}
        <link rel="icon" href="/logo.png" />
        
        {/* Configurações globais úteis para responsividade e SEO */}
        <meta name="viewport" content="width=device-width, initial-scale=1" />
      </Head>
      
      <Component {...pageProps} />
    </>
  );
}