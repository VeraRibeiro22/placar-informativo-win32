# Decisões Tomadas (Aspetos em Aberto)

Este ficheiro regista todas as decisões tomadas sobre aspetos que o enunciado deixa em aberto.

---

## D-001: Modelo de threads do `placar` (M1)

**Contexto:** O enunciado exige "pelo menos duas threads" mas não especifica a sua organização.

**Decisão:** Usar **3 fluxos de execução**:
- `main()` — cria recursos, lança threads, espera que terminem, faz cleanup
- Thread 1 (user) — lê comandos `ligar` / `desligar`
- Thread 2 (alerts) — espera no event `notificar`, lê alertas, gere waitable timer

**Justificação:** Separar o `main()` da lógica das threads permite um shutdown mais controlado. O `main()` funciona como "supervisor".

---

## D-002: Evento de shutdown

**Contexto:** O enunciado não especifica como a thread de alertas sabe que deve terminar quando o utilizador dá `desligar`.

**Decisão:** Usar um **evento anónimo auto-reset** (`hShutdown`) partilhado entre as threads via struct de contexto. A thread de alertas usa `WaitForMultipleObjects` para esperar simultaneamente no event de alerta e no event de shutdown.

**Justificação:** `WaitForMultipleObjects` é a forma idiomática de esperar por múltiplos sinais no Win32 sem polling.

---

## D-003: Geração de ID aleatório (M1)

**Contexto:** O enunciado diz "gerado aleatoriamente" mas não especifica o intervalo.

**Decisão:** `srand(GetTickCount()); id = (rand() % 100) + 1;` — gera um ID entre 1 e 100.

**Justificação:** Simples, cabe no `DWORD`, e é suficiente para demonstrar na M1. Na M2, será substituído por ID sequencial do `central`.

---

## D-004: Nome da chave Registry para o alerta (M1)

**Contexto:** O enunciado diz que o alerta é lido do Registry no par nome-valor cujo nome é igual ao nome do named pipe (ex: `"tubo"`), de tipo BINARY.

**Decisão:** Usar exatamente o nome do pipe como nome do valor REG_BINARY na mesma chave `HKCU\Software\TrabSO2`.

**Justificação:** Segue literalmente o que o enunciado describe.

---

## D-005: Estrutura do projeto Visual Studio

**Contexto:** O enunciado não especifica a organização dos projetos.

**Decisão:** Uma solução VS (`PlacarInformativo.sln`) com **3 projetos** separados (`placar`, `central`, `monitor`). Partilham o header `common/ipc_structs.h`.

**Justificação:** Facilita a navegação durante o desenvolvimento — tudo num único IDE. Cada programa compila de forma independente.
