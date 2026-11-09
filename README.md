# Placar Informativo (Sistema de Gestão de Alertas)
**Sistemas Operativos 2 2025/2026 - DEIS/ISEC**

## Turma: P2 - Grupo 
- Pedro Teixeira Amorim (2022157609)
- Vera Lúcia Martins Ribeiro (2024140140)

## Descrição

Um sistema baseado em comunicação interprocessual (IPC) em Windows para gestão e exibição de alertas de trânsito/informação. O sistema é composto por uma central de controlo e múltiplos placares, comunicando nativamente através das APIs Win32.

## Componentes
- **Central**: Interface de gestão, atribuição de IDs e difusão de alertas.
- **Placar**: Interface de exibição local, ligação à central e temporização automática de alertas.

## Tecnologias

- C (Win32 API)
- Named Pipes (Comunicação bidirecional assíncrona com OVERLAPPED I/O)
- Sincronização Win32 (Mutexes, Events, Semaphores, Critical Sections, Waitable Timers)
- MSVC (Visual Studio)

## Estrutura de Diretórios

- `Central/`: Código fonte do programa de controlo central.
- `placar/`: Código fonte do programa de exibição.
- `common/`: Estruturas de dados IPC partilhadas entre programas.
- `docs/`: Documentação e planeamento do projeto.

## Como Compilar e Executar

### Pré-requisitos

1. Microsoft Visual Studio (C/C++ Build Tools).
2. Sistema Operativo Windows.

### 1. Compilar o Projeto

Abrir o ficheiro `placar-informativo.sln` no Visual Studio e usar a opção Build/Compilar. Os binários serão gerados nas respetivas pastas `x64/Debug` ou `x64/Release`.

### 2. Executar a Simulação (3 Terminais)

Para testar a arquitetura IPC e a gestão concorrida, abre 3 terminais na pasta onde os binários foram gerados (ex: `x64\Debug`).

**Terminal 1: Central de Controlo**
*(Inicia o servidor de controlo e cria o Named Pipe com o nome fornecido)*
```bash
.\Central.exe tubo1
```

**Terminal 2: Placar 1**
*(Inicia a primeira instância do placar. De seguida, introduz o comando `ligar` para efetuar o registo)*
```bash
.\placar.exe tubo1
```

**Terminal 3: Placar 2**
*(Inicia um segundo placar em simultâneo. Introduz também o comando `ligar`)*
```bash
.\placar.exe tubo1
```

**Exemplo de Comandos a testar na Central (Terminal 1):**
```bash
alerta Acidente na A1 Km 34 60 0
listar
cancelar 1
encerrar
```

---
