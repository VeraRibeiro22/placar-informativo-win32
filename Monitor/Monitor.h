#pragma once
#include "resource.h"
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#include "../common/ipc_structs.h"

#define MAX_PLACARES 20
#define MAX_ALERTAS_DEFAULT 10
#define  SZPROGNAME _T("Monitor")

// ==========================================================================
// Structs
// ==========================================================================
typedef struct {
	//variáveis de configuracao (DialogBox)
	int maxAlertas;
	TCHAR nomeShm[256];
	TCHAR nomeMutex[256];
	TCHAR nomeEvento[256];

	HANDLE  hShm;						 // Handle do File Mapping
	HANDLE  hMutex;					    // Handle do Mutex
	HANDLE  hEvent;					    // Handle do Evento de notificação
	HANDLE  hShutdown;                  // Evento para terminar a thread
	HANDLE  hShmThread;					// Thread de leitura da SHM
	HWND    hWnd;						 // Handle da janela principal
} MONITOR_CONTEXT;
// ==========================================================================
// Pritótipos
// ==========================================================================

LRESULT CALLBACK WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ConfigDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);