# SensorFlow Firmware

![Linguagem](https://img.shields.io/badge/Linguagem-C-blue.svg)
![Plataforma](https://img.shields.io/badge/Plataforma-Raspberry%20Pi%20Pico%20W-green.svg)
![Licença](https://img.shields.io/badge/Licen%C3%A7a-MIT-yellow.svg)

Firmware para a placa Raspberry Pi Pico W projetado para ler dados de sensores de ambiente (temperatura, pressão e humidade) e enviá-los para um servidor remoto. Este projeto faz parte da solução SensorFlow.

---

## 📜 Visão Geral

O firmware inicializa os sensores, conecta-se a uma rede Wi-Fi e, em intervalos regulares, envia os dados coletados para uma API backend através de requisições HTTP POST. Cada dispositivo é unicamente identificado pelo ID da placa.

## ✨ Funcionalidades

-   **Leitura de Sensores:**
    -   Temperatura e Pressão com o sensor **BMP280**.
    -   Humidade com o sensor **AHT10**.
-   **Conectividade:**
    -   Conexão a redes Wi-Fi (WPA2-PSK).
    -   Comunicação com um servidor via TCP/IP.
-   **Envio de Dados:**
    -   Construção de corpo de requisição em formato JSON.
    -   Envio dos dados via método HTTP POST.
    -   Identificação única do dispositivo usando o ID da placa.
-   **Modularidade:**
    -   Drivers para os sensores separados da lógica principal da aplicação.

## 🛠️ Hardware Necessário

-   Raspberry Pi Pico W
-   Sensor de Pressão e Temperatura BMP280
-   Sensor de Humidade AHT10
-   Fios e protoboard para as conexões

### 🔌 Esquema de Ligação

| Sensor | Pino no Pico W |
| :--- | :--- |
| **BMP280 (I2C0)** | |
| VCC | 3V3 (OUT) |
| GND | GND |
| SCL | GP1 (I2C0_SCL) |
| SDA | GP0 (I2C0_SDA) |
| **AHT10 (I2C1)** | |
| VCC | 3V3 (OUT) |
| GND | GND |
| SCL | GP3 (I2C1_SCL) |
| SDA | GP2 (I2C1_SDA) |

## ⚙️ Configuração do Ambiente

Este projeto foi desenvolvido utilizando o SDK oficial do Raspberry Pi Pico e o Visual Studio Code.

### 1. Pré-requisitos

-   **VS Code**: Com a extensão [Raspberry Pi Pico](https://marketplace.visualstudio.com/items?itemName=RaspberryPi.raspberry-pi-pico).
-   **Pico SDK**: Siga o [guia oficial](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html) para instalar o SDK, a toolchain ARM GCC e as outras dependências.

### 2. Configuração do Projeto

1.  **Clone o repositório:**
    ```bash
    git clone [https://github.com/thalyssondev/sensorflow-firmware.git](https://github.com/thalyssondev/sensorflow-firmware.git)
    cd sensorflow-firmware
    ```

2.  **Crie o arquivo de segredos:**
    O projeto utiliza um arquivo `secrets.cmake` para armazenar informações sensíveis como credenciais de Wi-Fi e chaves de API. Este arquivo não é versionado pelo Git para sua segurança.

    -   Copie o arquivo de exemplo:
        ```bash
        cp secrets.cmake.example secrets.cmake
        ```
    -   Abra o arquivo `secrets.cmake` e preencha com as suas informações:
        ```cmake
        # Credenciais da rede Wi-Fi
        set(WIFI_SSID "NOME_DA_SUA_REDE")
        set(WIFI_PASSWORD "SENHA_DA_SUA_REDE")

        # Configurações do Servidor de Destino
        set(TARGET_SERVER_IP "192.168.1.100") # IP do seu servidor
        set(TARGET_PORT 8080)                 # Porta do seu servidor
        set(TARGET_PATH "/api/v1/data")       # Caminho da API
        set(API_KEY "SUA_CHAVE_DE_API_SECRETA")
        ```

## 🚀 Compilar e Enviar

Com o ambiente e o projeto configurados, você pode compilar e enviar o firmware para a sua Pico W.

1.  **Abra o projeto no VS Code.** A extensão do Raspberry Pi Pico deve detectar e configurar o projeto automaticamente.
2.  **Compile o projeto:**
    -   Pressione `F7` ou use a paleta de comandos (`Ctrl+Shift+P`) e execute `CMake: Build`.
3.  **Envie o firmware:**
    -   Coloque sua Pico W em modo `BOOTSEL` (segure o botão BOOTSEL e conecte o cabo USB).
    -   No VS Code, pressione `Shift+F5` para iniciar o upload (ou use a tarefa `Run Project`).
    -   Alternativamente, arraste o arquivo `build/main.uf2` para o drive `RPI-RP2` que aparece no seu computador.

## 📁 Estrutura do Projeto

```
.
├── .vscode/         # Configurações do VS Code (tasks, launch, settings)
├── build/           # Diretório dos arquivos de compilação (gerado)
├── inc/             # Bibliotecas e drivers dos sensores
│   ├── aht10/
│   └── bmp280/
├── src/             # Código fonte principal
│   └── main.c
├── CMakeLists.txt   # Script de compilação do CMake
├── lwipopts.h       # Configurações da pilha TCP/IP LwIP
├── secrets.cmake.example # Exemplo do arquivo de segredos
└── README.md        # Este arquivo
```

## 📄 Licença

Este projeto está sob a licença MIT. Veja o arquivo `LICENSE` para mais detalhes.
