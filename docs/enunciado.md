## ESPECIFICAÇÃO COMPLETA DO PROJETO

### 1. Contexto e Tema

O trabalho consiste na implementação de uma plataforma de divulgação de alertas para placares informativos rodoviários (semelhante aos das autoestradas portuguesas).

- Todos os processos correm **na mesma máquina**.
- Comunicação é **apenas entre processos** (IPC) – sem rede.
- Interfaces maioritariamente em **consola**, com exceção do monitor (interface gráfica Win32).
- Todos os processos são lançados pelo mesmo utilizador do SO.

---

### 2. Programas Envolvidos

#### 2.1 Programa `central`
- Agente central da plataforma.
- Atribui identificadores únicos aos placares.
- Centraliza o envio de alertas.
- Encerra o sistema de forma controlada, notificando todos.
- Interface de linha de comandos para o utilizador "administrador".
- **Apenas uma instância** pode estar a correr.
- Nada no sistema funciona sem o `central` estar ativo.

#### 2.2 Programa `placar`
- Lançado pelo utilizador "instalador" quando instala um placar.
- Regista-se no `central` e recebe um identificador único.
- Recebe e exibe alertas com temporização.
- Interface de linha de comandos para o utilizador "instalador".
- **Uma instância por placar** (podem existir vários em simultâneo, máximo 20).

#### 2.3 Programa `monitor`
- Monitoriza todos os alertas ativos em tempo real.
- **Interface gráfica Win32** obrigatória.
- Não interage com os placares – apenas com o `central` (via memória partilhada).
- Pode ser terminado a qualquer momento.
- **Pode haver várias instâncias** a correr em simultâneo.
- Não é necessário para o funcionamento da plataforma.

---

### 3. Lógica de Negócio

#### Placares
- Máximo de **20 placares** registados em simultâneo.
- Os placares podem ligar-se e desligar-se a qualquer momento.
- O identificador é um **DWORD**, começa em 1, incrementa de 1 em 1, **sem reutilização**.
- Quando recebe o seu identificador, o placar mostra-o na consola: `Identificador = 7`

#### Alertas
- Constituídos por: **mensagem de texto** (140 `TCHAR`) + **duração em segundos** (`DWORD`).
- **Só pode existir um alerta ativo por placar** em cada momento.
- Se chegar um novo alerta a um placar com alerta ativo, o anterior é cancelado/substituído.
- Ao receber um alerta, o placar mostra:
  ```
  23/03/2026 (13:51:10): 'A7 Norte – Acidente Km 34 - Lento'
  ```
- Quando o alerta expira (duração esgotada), o placar mostra:
  ```
  23/03/2026 (13:56:10): '---'
  ```

---

### 4. Comandos do Utilizador

#### Programa `central` (utilizador "administrador")

| Comando | Descrição |
|---|---|
| `alerta <msg> <duração> <id_placar>` | Envia alerta ao placar indicado. Se `id_placar = 0`, envia a todos os placares registados. Substitui alerta ativo, se existir. |
| `cancelar <id_placar>` | Cancela o alerta ativo no placar indicado. Sem efeito se não houver alerta ativo. |
| `listar` | Lista todos os placares registados e os seus identificadores. Se tiver alerta ativo, mostra-o também. |
| `encerrar` | Encerra a plataforma. Notifica todos os placares e monitores. |

#### Programa `placar` (utilizador "instalador")

| Comando | Descrição |
|---|---|
| `ligar` | Estabelece ligação com o `central`. Recebe e mostra o identificador atribuído. |
| `desligar` | Remove o placar da plataforma (informa o `central`) e termina o programa. |

#### Programa `monitor`
- Interface gráfica (Win32). Sem comandos de texto.
- Menu obrigatório com:
  - Redefinir número máximo de alertas a apresentar na lista.
  - Caixa de informações "Sobre" com dados do grupo.
  - Sair da aplicação.

---

### 5. Comunicação entre Programas

#### 5.1 `placar` ↔ `central` — via **Named Pipes**

##### Pedidos do `placar` para o `central`:
```c
typedef struct {
    BYTE tipo; // 1 – ligar, 2 – desligar, 3 – fim do alerta (duração esgotada)
} MSG_CMD;
```

##### Respostas/atualizações do `central` para o `placar`:
```c
// Atribuição de identificador
typedef struct {
    BYTE tipo;        // 3 – atribuir identificador
    DWORD identificador;
} MSG_ID;

// Novo alerta
typedef struct {
    BYTE tipo;        // 4 – novo alerta
    TCHAR msg[140];
    DWORD duracao;
} MSG_ALERTA;

// Cancelar alerta ou encerrar plataforma
typedef struct {
    BYTE tipo;        // 5 – cancelar alerta, 6 – encerrar plataforma
} MSG_CMD_CENTRAL;
```

> ⚠️ O nome do named pipe é especificado na linha de comandos do `placar` **ou** lido do Registry (ver secção 6).

#### 5.2 `monitor` ↔ `central` — via **Memória Partilhada (Ficheiros Mapeados em Memória)**

```c
typedef struct {
    DWORD identificador;
    DWORD duracao;
    TCHAR msg[140];
    BOOL desligar;
} SHM_ALERTA;
```

- A comunicação é **exclusivamente por memória partilhada**.
- Devem existir mecanismos (ex: eventos) para o `monitor` perceber quando há alterações (novo alerta, alerta removido, encerramento), **sem usar outros mecanismos de comunicação adicionais** para além dos da memória partilhada e sincronização.

---

### 6. Registry

O programa `placar` guarda/lê o nome do named pipe no Windows Registry:

- **Chave:** `HKEY_CURRENT_USER\Software\TrabSO2`
- **Par nome-valor:** `NPIPE` (tipo `REG_SZ`)

**Comportamento:**
- Se o nome do pipe for passado na linha de comandos → usar esse valor e guardá-lo no Registry (criar o par se não existir).
- Se não for passado na linha de comandos → ler do Registry.
- Se não estiver em lado nenhum → mostrar erro e **não iniciar o programa**.

---

### 7. Threads Obrigatórias no `placar`

O `placar` deve usar **pelo menos duas threads**:
1. **Thread de interação com o utilizador** — lê e processa comandos (`ligar`, `desligar`).
2. **Thread de receção de alertas e temporização** — fica à espera de alertas e gere a duração (com waitable timer).

As duas threads devem poder funcionar **em simultâneo**, sem bloquear uma à outra.

---

### 8. Aspetos em Aberto (a decidir pelos alunos)

Os seguintes aspetos não estão especificados e devem ser decididos de forma simples e lógica:
- Pormenores gráficos do monitor.
- Modelo de dados interno do `central` (para além do especificado).
- Mecanismos de sincronização adicionais (onde e quais são necessários).
- Quaisquer outros comportamentos não explicitamente descritos.

> **Diretriz:** Tomar decisões simples que não desvirtuem o sistema e que façam sentido do ponto de vista de SO.

---

### 9. Regras Importantes

- ❌ Não colocar ponteiros em memória partilhada.
- ❌ Não usar objetos STL (como `std::string`, `std::vector`) em estruturas de memória partilhada.
- ✅ Usar Unicode (`TCHAR`, `_T()`, funções `W` da API).
- ✅ Repositórios git devem ser **privados**.
- ✅ Código deve ser explicável em defesa oral — entender cada linha.
- ✅ `readme.txt` ou `readme.md` em cada entrega, indicando os requisitos implementados.
- ✅ Submeter em `.zip` apenas com ficheiros de código-fonte e projeto (sem binários, sem pasta `x64` ou `.vs`). Limite: 10MB.

---

### 10. Entregas e Prazos

| Entrega | Data | Conteúdo | Valor |
|---|---|---|---|
| **M1** | 12 de abril | Programa `placar` (versão standalone) | 0,75 val |
| **M2** | 10 de maio | Programa `central` + `placar` com comunicação real | 1,00 val |
| **M3** | 31 de maio | Programa `monitor` + `central` com memória partilhada | 0,75 val |

**Defesa oral individual (opcional):** aumenta a cotação máxima de 2,5 para 4 valores. Requer relatório de até 10 páginas entregue com a M3.

---

### 11. Requisitos Específicos da M1 (Programa `placar` standalone)

Para a primeira entrega, o `placar` deve funcionar **sem o `central`**, simulando os comportamentos:

1. Recebe comandos `ligar` e `desligar` do utilizador.
   - `ligar` → mostra um identificador **gerado aleatoriamente** (ex: `Identificador = 7`).
   - `desligar` → termina o programa de forma controlada (liberta todos os recursos).

2. Recebe e apresenta o nome do named pipe (via linha de comandos ou Registry). Exemplo de output: `NamedPipe = 'tubo'`. Cria o par `NPIPE` no Registry se ainda não existir.

3. Usa **duas threads**: uma para interação com o utilizador; outra para "simular" receção de alertas.

4. A "receção" de um alerta na M1 é simulada assim:
   - A thread de alertas fica à espera num **evento de reset manual** com nome `_T("notificar")`.
   - Quando o evento é sinalizado, lê o alerta do par nome-valor `"tubo"` (nome de exemplo, igual ao do named pipe) na chave `HKEY_CURRENT_USER\Software\TrabSO2`, de tipo **BINARY**, com conteúdo igual à struct `MSG_ALERTA`.
   - Apresenta na consola: `23/03/2026 (13:51:10): 'A7 Norte – Acidente Km 34 - Lento'`

5. Após receber o alerta, usa um **waitable timer** para gerir a duração e apresenta quando expirar: `23/03/2026 (13:56:10): '---'`
