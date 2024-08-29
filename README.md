# Controle Remoto de Nível de Tanque

## Descrição

Este projeto é um sistema de controle remoto para monitoramento e ajuste do nível de líquidos em tanques industriais. Desenvolvido como parte da disciplina de Lógica e Criatividade do curso de Engenharia de Software, o sistema permite a medição do nível do tanque, exibição em tempo real em um site e o ajuste desse nível através de controles online.

## Funcionalidades

- **Monitoramento em Tempo Real:** Visualize o nível atual do líquido no tanque diretamente no site.
- **Controle Remoto:** Ajuste o nível desejado do tanque com apenas um clique no site.
- **Interface Web:** Acesse e controle o sistema através de uma interface web intuitiva.

## Tecnologias Utilizadas

- **Frontend:**
    - **HTML:** Estrutura da página web.
    - **CSS:** Estilização e design da interface.
    - **JavaScript:** Funcionalidade e interação do usuário.
- **Backend:**
    - **NodeMCU ESP8266:** Placa de desenvolvimento com capacidade de Wi-Fi para comunicação com o servidor e controle das bombas.
    - **Arduino Nano:** Placa adicional para controle e leitura de sensores.
- **Hospedagem:**
    - **Vercel:** Plataforma de hospedagem para o site.

## Arquitetura

O sistema é composto por duas placas Arduino conectadas entre si e ao site:

1. **ESP8266 NodeMCU V3:** Gerencia a comunicação com o site e controla as bombas do tanque através de comandos recebidos pela interface web.
2. **Arduino Nano:** Complementa a ESP8266 na leitura dos sensores de nível e envio dessas informações para a placa NodeMCU.

## Instalação e Configuração

1. **Clone o Repositório:**
    
    ```bash
    git clone <https://github.com/coderod3/controletanque.git>
    cd controletanque
    
    ```
    
2. **Configuração do Site:**
    - Modifique as configurações de conexão no código do frontend, se necessário.
    - Certifique-se de que o projeto está configurado corretamente no Vercel. As instruções para o deploy podem ser encontradas na [documentação do Vercel](https://vercel.com/docs).
3. **Configuração do Hardware:**
    - Conecte a placa NodeMCU e o Arduino Nano de acordo com o diagrama de conexão fornecido no projeto.
    - Carregue os códigos adequados em ambas as placas. Os códigos podem ser encontrados na pasta `firmware` do repositório.
4. **Conexão e Testes:**
    - Verifique se as placas estão conectadas à rede Wi-Fi.
    - Acesse o site para verificar se a interface está se comunicando corretamente com as placas e controlando o nível do tanque.

## Contribuições

Contribuições são bem-vindas! Se você quiser ajudar a melhorar este projeto, por favor, siga as diretrizes abaixo:

- **Fork o Repositório**
- **Crie uma Branch para sua Feature (`git checkout -b feature/MinhaFeature`)**
- **Faça Commit das suas Alterações (`git commit -am 'Adiciona uma nova feature'`)**
- **Push para a Branch (`git push origin feature/MinhaFeature`)**
- **Abra um Pull Request**

## Licença

Este projeto está licenciado sob a [Licença MIT].

## Contato

Para mais informações ou dúvidas, entre em contato com:

- Nome do Autor: [Rodrigo Araujo]
- Email: [rodrigo.araujo9013@gmail.com]
- GitHub: [https://github.com/coderod3/]
