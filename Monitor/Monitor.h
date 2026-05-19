#pragma once
#include "resource.h"
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#include "../common/ipc_structs.h"

#define MAX_PLACARES 20
#define MAX_ALERTAS_DEFAULT 3
#define  SZPROGNAME _T("Monitor")

// ==========================================================================
// Structs
// ==========================================================================
typedef struct {
	//vari�veis de configuracao (DialogBox)
	int maxAlertas;
	int numAlertaAtivos;
	struct {
		DWORD identificador;
		DWORD duracao;
		TCHAR msg[140];
	} alertasLocais[MAX_PLACARES];

	int offset;
	HANDLE hConnectThread;
	TCHAR nomeShm[256];
	TCHAR nomeMutex[256];
	TCHAR nomeEvento[256];
	BOOL centralLigada;
	SHM_ALERTA* pShm;
	HANDLE  hShm;						 // Handle do File Mapping
	HANDLE  hMutex;					    // Handle do Mutex
	HANDLE  hEvent;					    // Handle do Evento de notifica��o
	HANDLE  hShutdown;                  // Evento para terminar a thread
	HANDLE  hShmThread;					// Thread de leitura da SHM
	HWND    hWnd;						 // Handle da janela principal
} MONITOR_CONTEXT;
// ==========================================================================
// Prit�tipos
// ==========================================================================

LRESULT CALLBACK WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ConfigDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
unsigned __stdcall ShmReadThread(void* param);
unsigned __stdcall ConnectThread(void* param);
