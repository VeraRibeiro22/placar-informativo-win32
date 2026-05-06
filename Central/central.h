#pragma once

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <process.h>

#include "../common/ipc_structs.h"

// ==========================================================================
// Constantes
// ==========================================================================

// Nome do pipe a ser utilizado
#define PIPE_NAME            _T("\\\\.\\pipe\\%s")
#define MAX_PLACARES        20
#define MUTEX_CENTRAL     _T("CentralControlo_Mutex")
#define PIPE_BUFFER_SIZE    4096

// ==========================================================================
// Structs
// ==========================================================================
typedef struct {
	BOOL   ativo;               // TRUE se estiver ativo
	DWORD  identificador;       // ID único atribuído
	HANDLE hPipe;               // Handle do named pipe para este placar
	BOOL   temAlerta;           // TRUE se tem alerta ativo
	TCHAR  alertaMsg[140];      // Mensagem do alerta ativo
	DWORD  alertaDuracao;       // Duração do alerta ativo (segundos)
} PLACAR_INFO;


typedef struct {
	TCHAR pipeName[256];
	TCHAR pipePath[256];
	PLACAR_INFO     placares[MAX_PLACARES];
	DWORD           numPlacares;            
	DWORD           proximoId;  // próximo id a atribuir (começa em 1)
	CRITICAL_SECTION csPlacares;           // protege o array de placares
	HANDLE           hShutdown;            // Evento que sinaliza todas as threads para terminar
}CENTRAL_CONTEXT;

typedef struct {
	CENTRAL_CONTEXT* ctx;
	HANDLE           hPipe;     // Pipe já conectado a este placar
	int              posicao;      // Índice no array placares (-1 enquanto não registado)
} PLACAR_THREAD_DATA;

// ==========================================================================
// Protótipos
// ==========================================================================

// Thread para aceitar placares
unsigned __stdcall AcceptThread(void* param);
unsigned __stdcall CommandThread(void* param);

// Protótipos das threads da Central
unsigned __stdcall AcceptThread(void* param);
unsigned __stdcall CommandThread(void* param);
//unsigned __stdcall PlacarHandlerThread(void* param);
