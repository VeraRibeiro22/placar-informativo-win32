#include "monitor.h"
unsigned __stdcall ConnectThread(void* param) {
	MONITOR_CONTEXT* pCtx = (MONITOR_CONTEXT*)param;

	// criar ou abrir o evento Placar_Central_Ready
    HANDLE hReady = CreateEvent(NULL, TRUE, FALSE, _T("Placar_Central_Ready"));
    if (hReady == NULL) {
        InvalidateRect(pCtx->hWnd, NULL, TRUE);
        return 0;
    }
    //bloqueia até a central estar pronta
    WaitForSingleObject(hReady, INFINITE);
    CloseHandle(hReady);

    pCtx->hShm = OpenFileMapping(FILE_MAP_READ, FALSE, pCtx->nomeShm);
    pCtx->pShm = (SHM_ALERTA*)MapViewOfFile(pCtx->hShm, FILE_MAP_READ, 0, 0, sizeof(SHM_ALERTA));
    pCtx->hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, pCtx->nomeMutex);
    pCtx->hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, pCtx->nomeEvento);
    if (pCtx->pShm && pCtx->hMutex && pCtx->hEvent) {
        pCtx->centralLigada = TRUE;
        pCtx->hShmThread = (HANDLE)_beginthreadex(NULL, 0, ShmReadThread, pCtx, 0, NULL);
    }
    InvalidateRect(pCtx->hWnd, NULL, TRUE);
    return 0;

}