# Reasoning Plan — Placar Informativo

## Milestone Decomposition

### M1 — `placar` standalone (deadline: 12 abril)

| # | Sub-task | Win32 Mechanism | Structs Involved |
|---|---------|----------------|-----------------|
| 1.1 | Parse command-line args for pipe name | `__argc` / `__argv` / `GetCommandLineW` | — |
| 1.2 | Read/Write Registry key `HKCU\Software\TrabSO2\NPIPE` | `RegCreateKeyEx`, `RegSetValueEx`, `RegQueryValueEx` | — |
| 1.3 | Launch user-interaction thread (main or secondary) | `_beginthreadex` / `CreateThread` | — |
| 1.4 | Handle `ligar` command → generate random ID, display it | `rand()` / `srand(GetTickCount())` | — |
| 1.5 | Handle `desligar` command → signal shutdown, clean up | `SetEvent` (shutdown event) | — |
| 1.6 | Launch alert-reception thread | `_beginthreadex` | — |
| 1.7 | Wait on named manual-reset event `_T("notificar")` | `CreateEvent` (manual-reset), `WaitForSingleObject` / `WaitForMultipleObjects` | — |
| 1.8 | On event signal → read `MSG_ALERTA` from Registry (BINARY) | `RegQueryValueEx` (REG_BINARY) | `MSG_ALERTA` |
| 1.9 | Display alert with timestamp format | `GetLocalTime`, `_tprintf` | `MSG_ALERTA` |
| 1.10 | Create Waitable Timer for alert duration | `CreateWaitableTimer`, `SetWaitableTimer` | — |
| 1.11 | On timer expiry → display `'---'` with timestamp | `WaitForSingleObject` on timer | — |
| 1.12 | Handle new alert replacing active alert (cancel timer, reset) | `CancelWaitableTimer` | `MSG_ALERTA` |
| 1.13 | Graceful shutdown — close all handles, join threads | `CloseHandle`, `WaitForSingleObject` on thread | — |

### M2 — `central` + `placar` real communication (deadline: 10 maio)

| # | Sub-task | Win32 Mechanism | Structs Involved |
|---|---------|----------------|-----------------|
| 2.1 | `central`: Create Named Pipe server (multi-instance) | `CreateNamedPipe` (overlapped or per-thread) | — |
| 2.2 | `central`: Accept connections, assign unique DWORD IDs | `ConnectNamedPipe`, ID counter | `MSG_CMD`, `MSG_ID` |
| 2.3 | `central`: Parse admin commands (`alerta`, `cancelar`, `listar`, `encerrar`) | Console input parsing | — |
| 2.4 | `central`: Route alerts to specific placar or broadcast (id=0) | `WriteFile` on pipe | `MSG_ALERTA` |
| 2.5 | `central`: Handle `cancelar` → send tipo=5 | `WriteFile` | `MSG_CMD_CENTRAL` |
| 2.6 | `central`: Handle `encerrar` → send tipo=6 to all, shutdown | `WriteFile` to all | `MSG_CMD_CENTRAL` |
| 2.7 | `central`: Track connected placares (max 20) | Internal array/struct | — |
| 2.8 | `central`: Handle placar disconnection (tipo=2, tipo=3) | `ReadFile` | `MSG_CMD` |
| 2.9 | `central`: Single instance enforcement | Named Mutex | — |
| 2.10 | `placar`: Replace M1 simulation with real Named Pipe client | `CreateFile` on `\\.\pipe\<name>` | — |
| 2.11 | `placar`: Send `ligar` (tipo=1) via pipe, receive `MSG_ID` | `WriteFile`, `ReadFile` | `MSG_CMD`, `MSG_ID` |
| 2.12 | `placar`: Alert thread reads from pipe instead of Registry | `ReadFile` (blocking/overlapped) | `MSG_ALERTA`, `MSG_CMD_CENTRAL` |
| 2.13 | `placar`: Handle tipo=5 (cancel), tipo=6 (shutdown) | — | `MSG_CMD_CENTRAL` |
| 2.14 | `placar`: Send tipo=3 (alert expired) to central | `WriteFile` | `MSG_CMD` |

### M3 — `monitor` + shared memory (deadline: 31 maio)

| # | Sub-task | Win32 Mechanism | Structs Involved |
|---|---------|----------------|-----------------|
| 3.1 | `central`: Create shared memory region | `CreateFileMapping`, `MapViewOfFile` | `SHM_ALERTA[]` |
| 3.2 | `central`: Update shared memory on alert changes | Direct memory write + signal event | `SHM_ALERTA` |
| 3.3 | `central`: Signal monitors on shutdown (`desligar = TRUE`) | Event + shared memory flag | `SHM_ALERTA` |
| 3.4 | `monitor`: Win32 GUI window (RegisterClass, CreateWindow, WndProc) | Win32 GUI API | — |
| 3.5 | `monitor`: Open shared memory (read-only) | `OpenFileMapping`, `MapViewOfFile` | `SHM_ALERTA[]` |
| 3.6 | `monitor`: Wait for update events, refresh display | `WaitForSingleObject` or `MsgWaitForMultipleObjects` | — |
| 3.7 | `monitor`: Menu — redefine max alerts, "Sobre", exit | `CreateMenu`, `AppendMenu`, `WM_COMMAND` | — |
| 3.8 | `monitor`: Display active alerts list in window | `WM_PAINT` / ListView / custom draw | `SHM_ALERTA` |

## Open Decisions Identified

1. **Alert thread model in M1**: Main thread = user commands; secondary thread = alert reception. (Spec confirms this.)
2. **Shutdown coordination**: Need a named or anonymous event for signaling the alert thread to exit. Proposal: anonymous auto-reset event `hShutdown`.
3. **WaitForMultipleObjects in alert thread**: Wait on both `hNotificar` (alert event) and `hShutdown` (exit event) simultaneously.
4. **M1 alert from Registry — key name**: The spec says the key name is the same as the pipe name (e.g., `"tubo"`). The value type is `REG_BINARY` containing a `MSG_ALERTA` struct.
5. **ID generation in M1**: `rand()` seeded with time — spec says "gerado aleatoriamente".
6. **`central` thread model (M2)**: One thread per connected placar, or thread pool? Simplest: one thread per placar (max 20).
7. **Monitor shared memory layout (M3)**: Array of `SHM_ALERTA[20]` (one per possible placar). Simplest approach.

## Confidence Check

- ✅ Spec fully read and cross-referenced
- ✅ No existing source code — starting from scratch
- ✅ All M1 requirements are self-contained (no `central` dependency)
- ✅ Structs are exact and cannot be modified
