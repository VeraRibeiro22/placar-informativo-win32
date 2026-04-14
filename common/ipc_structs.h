/**
 * ipc_structs.h — Estruturas de comunicação IPC
 * 
 * Ficheiro partilhado por todos os programas (placar, central, monitor).
 * Define as structs EXATAS do enunciado — NÃO MODIFICAR.
 * 
 * Convenções:
 *   - Todos os campos usam tipos Win32 (BYTE, DWORD, TCHAR, BOOL)
 *   - Nenhuma struct contém ponteiros (proibido em memória partilhada)
 *   - Unicode: TCHAR é wchar_t quando UNICODE está definido
 */

#pragma once

#include <windows.h>
#include <tchar.h>

/* ==========================================================================
 * placar ↔ central  (via Named Pipes)
 * Tipo 1 = ligar, 2 = desligar, 3 = fim do alerta (duração esgotada), 5 = cancelar alerta, 6 = encerrar plataforma
 * ========================================================================== */
typedef struct {
    BYTE tipo;      // 1 – ligar | 2 – desligar | 3 – fim do alerta| 5 – cancelar alerta | 6 – encerrar plataforma
} MSG_CMD;

/* ==========================================================================
 * central → placar : atribuição de identificador  (via Named Pipes)
 * ========================================================================== */
typedef struct {
    BYTE tipo;              // 3 – atribuir identificador
    DWORD identificador;    // ID único atribuído pelo central (começa em 1, sem reutilização)
} MSG_ID;

/* ==========================================================================
 * central → placar : novo alerta  (via Named Pipes)
 * Na M1, esta struct é lida do Registry (REG_BINARY) para simulação
 * ========================================================================== */
typedef struct {
    BYTE tipo;          // 4 – novo alerta
    TCHAR msg[140];     // Mensagem do alerta (máximo 140 TCHAR)
    DWORD duracao;      // Duração em segundos
} MSG_ALERTA;



/* ==========================================================================
 * central ↔ monitor  (via Memória Partilhada / Ficheiros Mapeados em Memória)
 * ========================================================================== */
typedef struct {
    DWORD identificador;    // ID do placar a que pertence o alerta
    DWORD duracao;          // Duração em segundos
    TCHAR msg[140];         // Mensagem do alerta
    BOOL desligar;          // TRUE quando o central está a encerrar
} SHM_ALERTA;
