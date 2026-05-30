#include "monitor.h"
unsigned __stdcall ConnectThread(void* param) {
    MONITOR_CONTEXT* pCtx = (MONITOR_CONTEXT*)param;

    while (1) {
        // Criar ou abrir o evento "Placar_Central_Ready"
        HANDLE hReady = CreateEvent(NULL, TRUE, FALSE, _T("Placar_Central_Ready"));
        if (hReady == NULL) {
            InvalidateRect(pCtx->hWnd, NULL, TRUE);
            return 0;
        }
        // Esperar pela Central OU pelo shutdown do Monitor
        HANDLE handles[2] = { hReady, pCtx->hShutdown };
        DWORD result = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        CloseHandle(hReady);
        if (result == WAIT_OBJECT_0 + 1) {
            // Monitor está a fechar
            return 0;
        }
        // Tentar abrir os recursos da Central
        pCtx->hShm = OpenFileMapping(FILE_MAP_READ, FALSE, pCtx->nomeShm);
        if (pCtx->hShm)
            pCtx->pShm = (SHM_ALERTA*)MapViewOfFile(pCtx->hShm, FILE_MAP_READ, 0, 0, sizeof(SHM_ALERTA));
        pCtx->hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, pCtx->nomeMutex);
        pCtx->hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, pCtx->nomeEvento);
        if (pCtx->pShm && pCtx->hMutex && pCtx->hEvent) {
            // Tudo aberto com sucesso - Central está mesmo pronta
            pCtx->centralLigada = TRUE;
            pCtx->hShmThread = (HANDLE)_beginthreadex(NULL, 0, ShmReadThread, pCtx, 0, NULL);
            InvalidateRect(pCtx->hWnd, NULL, TRUE);
            return 0;
        }
        // Evento estava "stale" (sinalizado da sessao anterior)
        // Limpar handles parciais e voltar a tentar
        if (pCtx->pShm) { UnmapViewOfFile(pCtx->pShm); pCtx->pShm = NULL; }
        if (pCtx->hShm) { CloseHandle(pCtx->hShm);     pCtx->hShm = NULL; }
        if (pCtx->hMutex) { CloseHandle(pCtx->hMutex);   pCtx->hMutex = NULL; }
        if (pCtx->hEvent) { CloseHandle(pCtx->hEvent);   pCtx->hEvent = NULL; }
        // Volta ao topo do while:
        // - A Central ja terminou, por isso o evento foi destruido
        // - CreateEvent cria um NOVO evento (unsignaled)
        // - WaitForMultipleObjects bloqueia corretamente ate nova Central arrancar
    }
}