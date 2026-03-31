#pragma once

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <process.h>

#include "../common/ipc_structs.h"

// ==========================================================================
// Constantes
// ==========================================================================

// Chave do Registry onde se guarda/lê o nome do pipe
#define REGISTRY_KEY            _T("Software\\TrabSO2")

// Nome do valor REG_SZ que guarda o nome do pipe
#define REGISTRY_NPIPE_VALUE    _T("NPIPE")

// Nome do evento manual-reset para simular chegada de alerta
#define EVENT_NOTIFICAR_NAME    _T("notificar")

#define MAX_PIPE_NAME           256


// ==========================================================================
// Structs
// ==========================================================================

typedef struct {
    // Handles de sincronização
    HANDLE hShutdown;       // Evento anónimo auto-reset: sinaliza todas as threads para terminar
    HANDLE hNotificar;      // Evento manual-reset com nome "notificar" para simular chegada de alerta
    HANDLE hTimer;          // Waitable timer: gere a duração do alerta ativo

    // Dados do placar
    TCHAR  pipeName[MAX_PIPE_NAME];
    BOOL   isConnected;
    DWORD  identificador;
} PLACAR_CONTEXT;


// ==========================================================================
// Protótipos
// ==========================================================================

// Thread do utilizador para ler comandos da consola
unsigned __stdcall UserThread(void* param);

// Thread de alertas para esperar por eventos e gerir o waitable timer
unsigned __stdcall AlertThread(void* param);


/**
 * Resolve o nome do named pipe
 *   1. Se argc >= 2 → usa argv[1], guarda no Registry
 *   2. Senão → lê do Registry
 *   3. Se não encontrar em lado nenhum → retorna FALSE
 *
 * @param argc
 * @param argv
 * @param outPipeName Buffer de saída
 * @param size        Tamanho do buffer em caracteres (TCHAR)
 * @return TRUE se resolvido com sucesso
 */
BOOL ResolvePipeName(int argc, TCHAR* argv[], TCHAR* outPipeName, DWORD size);

/**
 * Lê uma struct MSG_ALERTA do Registry (tipo REG_BINARY)
 *
 * @param pipeName  Nome do valor no Registry
 * @param outAlert  Ponteiro para struct MSG_ALERTA de saída
 * @return TRUE se lido com sucesso
 */
BOOL ReadAlertFromRegistry(const TCHAR* pipeName, MSG_ALERTA* outAlert);

/* ==========================================================================
 * Protótipos — Utilidades
 * ========================================================================== */

/**
 * Formata a data/hora atual no formato DD/MM/AAAA (HH:MM:SS).
 *
 * @param buffer  Buffer de saída
 * @param size    Tamanho do buffer em caracteres (TCHAR)
 */
void FormatTimestamp(TCHAR* buffer, DWORD size);
