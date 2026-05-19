#include "monitor.h"
unsigned __stdcall ShmReadThread(void* param) {
	MONITOR_CONTEXT* pCtx = (MONITOR_CONTEXT*)param;

	//esperar eventos:
	// 0 - hEvent (central atualizou os dados)
	// 1 - hShutdown (monitor vai encerrar)

	HANDLE eventos[2] = { pCtx->hEvent, pCtx->hShutdown };

	while (1) {

		DWORD waitResult = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
		if (waitResult == WAIT_OBJECT_0 + 1) {
			// Acordou pelo hShutdown (Monitor a fechar)
			break;
		}
		else if (waitResult == WAIT_OBJECT_0) {
			// Acordou pelo hEvent da Central

			//trancar a Shm
			WaitForSingleObject(pCtx->hMutex, INFINITE);

			//verifica se a central vai encerrar
			if (pCtx->pShm->desligar) {
				pCtx->centralLigada = FALSE;
				pCtx->numAlertaAtivos = 0;
				ReleaseMutex(pCtx->hMutex);

				// Fechar os handles antigos
				UnmapViewOfFile(pCtx->pShm);  pCtx->pShm = NULL;
				CloseHandle(pCtx->hShm);      pCtx->hShm = NULL;
				CloseHandle(pCtx->hMutex);    pCtx->hMutex = NULL;
				CloseHandle(pCtx->hEvent);    pCtx->hEvent = NULL;
				//atualizar janela
				InvalidateRect(pCtx->hWnd, NULL, TRUE);
				// Relança a ConnectThread para esperar a Central voltar
				pCtx->hConnectThread = (HANDLE)_beginthreadex(NULL, 0, ConnectThread, pCtx, 0, NULL);
				break;
			}
			//copiar a SHM - copia local para o WM_PAINT
			pCtx->numAlertaAtivos = 0;
			for (int i = 0; i < MAX_PLACARES; i++) {
				if (pCtx->pShm->placar[i].identificador != 0) {
					int j = pCtx->numAlertaAtivos++;
					pCtx->alertasLocais[j].identificador = pCtx->pShm->placar[i].identificador;
					pCtx->alertasLocais[j].duracao = pCtx->pShm->placar[i].duracao;
					_tcscpy_s(pCtx->alertasLocais[j].msg, 140, pCtx->pShm->placar[i].msg);
				}
			}

			ReleaseMutex(pCtx->hMutex);
			ResetEvent(pCtx->hEvent);
			//atualizar janela no WM_PAINT
			InvalidateRect(pCtx->hWnd, NULL, TRUE);
		}
	}
	ExitThread(0);
}
