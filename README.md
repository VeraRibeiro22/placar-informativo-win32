# SO2 – Trabalho Laboratorial – M1

**Grupo:** Pedro Teixeira Amorim (2022157609) Vera Lúcia Martins Ribeiro (2024)
**Turma:** P2
**Data de entrega:** 12 de abril de 2026

---

## Requisitos Implementados (M1 — `placar` standalone)

- [x] Comando `ligar` — gera identificador aleatório e apresenta `Identificador = X`
- [x] Comando `desligar` — termina o programa de forma controlada, libertando todos os handles
- [x] Nome do named pipe via linha de comandos (`placar.exe tubo`) ou lido do Registry
- [x] Guarda o nome do pipe em `HKCU\Software\TrabSO2` → `NPIPE` (`REG_SZ`) quando passado na CLI
- [x] Erro e não inicia se o nome do pipe não for encontrado em lado nenhum
- [x] Output `NamedPipe = 'tubo'` ao iniciar
- [x] Duas threads obrigatórias: `UserThread` (comandos) e `AlertThread` (alertas + timer)
- [x] Thread de alertas espera em evento manual-reset com nome `"notificar"`
- [x] Lê alerta do Registry como `REG_BINARY` (struct `MSG_ALERTA`) — par nome = nome do pipe
- [x] Apresenta alerta com formato `DD/MM/AAAA (HH:MM:SS): 'mensagem'`
- [x] Usa waitable timer para gerir a duração do alerta
- [x] Apresenta `'---'` com timestamp quando o alerta expira
- [x] Novo alerta substitui o alerta ativo (cancela timer anterior, reinicia com nova duração)

---

## Decisões Tomadas

| Decisão | Escolha |
|---|---|
| Organização das threads | `main()` como supervisor + `UserThread` + `AlertThread` |
| Shutdown inter-thread | Evento anónimo auto-reset (`hShutdown`) via `WaitForMultipleObjects` |
| Intervalo do ID aleatório | 1–100 (`rand() % 100 + 1`) |
| Nome do valor Registry para o alerta | Igual ao nome do pipe (segue literalmente o enunciado) |

---

### Como Executar

**1. Lançar o placar com nome do pipe:**
```
placar.exe tubo
```

**2. Na consola do placar, dar o comando:**
```
ligar
```
→ Output: `Identificador = 7` (valor aleatório)

**3. Simular chegada de alerta** (script `injector.c`):
- Escreve uma struct `MSG_ALERTA` no Registry em `HKCU\Software\TrabSO2`, valor `tubo`, tipo `REG_BINARY`
- Sinaliza o evento nomeado `"notificar"` com `SetEvent` 

```
tools\injetor.exe tubo "A7 Norte - Acidente Km 34 - Lento" 15
```

**4. Terminar:**
```
desligar
```

---