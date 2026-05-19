#pragma once

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>

#include "../common/ipc_structs.h"

// ==========================================================================
// Constantes
// ==========================================================================

// Nome do pipe a ser utilizado
#define PIPE_NAME            _T("\\\\.\\pipe\\%s")
#define MAX_PLACARES        20
#define MUTEX_CENTRAL     _T("CentralControlo_Mutex")
#define PIPE_BUFFER_SIZE    4096

// Nomes dos recursos IPC partilhados com o monitor
#define SHM_NAME                _T("Placar_Shared_Memory")
#define SHM_MUTEX_NAME          _T("Placar_Mutex_SHM")
#define SHM_EVENT_NAME          _T("Placar_Event_Update")
#define SHM_READY_EVENT_NAME    _T("Placar_Central_Ready")


// ==========================================================================
// Structs
// ==========================================================================
typedef struct {
	BOOL   ativo;               // TRUE se estiver ativo
	DWORD  identificador;       // ID �nico atribu�do
	HANDLE hPipe;               // Handle do named pipe para este placar
	BOOL   temAlerta;           // TRUE se tem alerta ativo
	TCHAR  alertaMsg[140];      // Mensagem do alerta ativo
	DWORD  alertaDuracao;       // Dura��o do alerta ativo (segundos)
} PLACAR_INFO;


typedef struct {
	TCHAR pipeName[256];
	TCHAR pipePath[256];
	PLACAR_INFO     placares[MAX_PLACARES];
	DWORD           numPlacares;            
	DWORD           proximoId;  // pr�ximo id a atribuir (come�a em 1)
	CRITICAL_SECTION csPlacares;           // protege o array de placares
	HANDLE           hShutdown;            // Evento que sinaliza todas as threads para terminar
	HANDLE           hSemPlacares;         // Semaforo para controlar vagas

	HANDLE           hShm;
	SHM_ALERTA* pShm;         
	HANDLE           hShmMutex;     // protege a SHM
	HANDLE           hShmEvent;     // evento notifica o monitor de alterações
	HANDLE           hReadyEvent;   // evento sinalizado quando SHM está pronta
}CENTRAL_CONTEXT;

typedef struct {
	CENTRAL_CONTEXT* ctx;
	HANDLE           hPipe;     // Pipe j� conectado a este placar
	int              posicao;      // �ndice no array placares (-1 enquanto n�o registado)
} PLACAR_THREAD_DATA;

// ==========================================================================
// Prot�tipos
// ==========================================================================

// Thread para aceitar placares
unsigned __stdcall AcceptThread(void* param);
unsigned __stdcall CommandThread(void* param);

// Prot�tipos das threads da Central
unsigned __stdcall AcceptThread(void* param);
unsigned __stdcall CommandThread(void* param);
//unsigned __stdcall PlacarHandlerThread(void* param);
