# Título do Projeto  

Estação Meteorológica

## Objetivo Geral  

O projeto tem como objetivo realizar o monitoramento remoto de parâmetros ambientais ( temperatura, umidade e pressão atmosférica ) por meio de um servidor web acessível via navegador. A estação permite a visualização em tempo real dos dados, o acompanhamento gráfico da variação dos parâmetros e a configuração dinâmica de limites operacionais, auxiliando na análise e tomada de decisões sem a necessidade de presença física no local.

## Descrição Funcional

O sistema funciona a partir da coleta de dados ambientais pelos sensores AHT10 e BMP280, integrados à placa BitDogLab com o microcontrolador RP2040. A leitura de temperatura é obtida pela média entre os dois sensores, enquanto umidade e pressão são capturadas individualmente. Os dados são processados e disponibilizados por um servidor web implementado com a biblioteca LWIPOPTS, que oferece quatro rotas principais:

- `GET /` – retorna a interface HTML da página;

- `GET /data` – fornece os dados atuais de temperatura, umidade e pressão;

- `GET /config` – retorna os parâmetros de configuração (limites e offsets);

- `POST /config` – atualiza os parâmetros de configuração conforme input do usuário.

A cada 2 segundos, os dados são atualizados via a rota /data e armazenados em uma janela de aproximadamente 10 minutos, permitindo a visualização de três gráficos dinâmicos (um para cada grandeza) e dos valores atuais. A interface também permite ao usuário ajustar os valores de máximo, mínimo e offset de cada grandeza diretamente pelo navegador.

Localmente, na BitDogLab, são apresentadas duas telas para o usuário:

- **Informações gerais** - essa tela é responsável por mostrar os dados de inicialização e de Wi-Fi, bem como o IP do dispositivo na rede
- **Informações de sensores** - já essa tela é responsável apenas por indicar os valores lidos dos sensores.

As telas podem ser alteradas através do botão A, que muda a visualização no display LCD.

Ainda localmente, é possível visualizar o modo de operação da estação, através do LED RGB, da Matriz de LEDS RGB e do Buzzer. Estes indicam se os limites mínimos ou máximos foram ultrapassados por algum parâmetro.

- Verde (LED + Matriz) e buzzer desligado indicam funcionamento normal.

- Vermelho (LED + Matriz) e buzzer ativado indicam que algum parâmetro excedeu os limites configurados.

## BitDogLab e Código

Na BitDogLab, a lógica principal é executada por polling, exceto a mudança de telas, que é tratada por interrupção externa. Seguindo, foram utilizados os seguintes periféricos:

- **Matriz de LEDs RGB**: utilizada exclusivamente para um indicativo visual do estado da estação meteorológica.
- **LED RGB**: Com o mesmo propósito da matriz, o **LED RGB** foi incorporado para indicar o estado de funcionamento.
- **Buzzer**: Com o intuito de alertar caso os limites dos parâmetros medidos sejam ultrapassados, o **Buzzer** foi usado para emitir um sinal.
- **Display LCD**: Apresenta informações importantes, além de possuir duas telas de dados que pode ser alterada usando um botão. Ele apresenta dados de WI-Fi, bem como dados capturados por sensores.
- **Botão A**: botão utilizado para acionar a interrupção externa, capaz de alterar a exibição do **Display LCD**
- **AHT10**: Sensor de temperatura e umidade que foi utilizado via comunicação **I2C**.
- **BMP280**: Sensor de temperatura e pressão que foi utilizado via comunicação **I2C**.
- **Módulo Wi-Fi**: utilizado exclusivamente para que fosse possível subir um **web-server** no **RP2040**.

A BitDogLab trabalha com esses periféricos para que seja possível levar os dados obtidos localmente para a página do cliente, na **Web**, além também de indicar o estado do projeto com indicativos visuais e sonoros.

Como foi dito, a lógica dos indicativos é totalmente feita em pooling, tendo verificações feitas com if para acionar ou não os periféricos necessários para indicar o estado do sistema naquele momento.

Além disso, é utilizada a biblioteca LWIPOPTS para configurar e operar um **web-server** capaz de fazer todas as rotas e funcionalidades descritas neste documento.
## Links para ao vídeo.
Link vídeo ensaio:;
