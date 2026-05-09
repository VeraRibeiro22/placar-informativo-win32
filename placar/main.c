#include "placar.h"

BOOL EnviarMensagemCentral(HANDLE hPipe, void* pData, DWORD cbData) {
    if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    OVERLAPPED ov;
    ZeroMemory(&ov, sizeof(OVERLAPPED));
    ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ov.hEvent == NULL) return FALSE;

    DWORD bytesWritten;
    BOOL bResult = WriteFile(hPipe, pData, cbData, &bytesWritten, &ov);

    if (!bResult && GetLastError() == ERROR_IO_PENDING) {
        // Espera pela operacao overlapped terminar
        DWORD waitRes = WaitForSingleObject(ov.hEvent, INFINITE);
        if (waitRes == WAIT_OBJECT_0) {
            bResult = GetOverlappedResult(hPipe, &ov, &bytesWritten, FALSE);
        } else {
            CancelIo(hPipe);
            bResult = FALSE;
        }
    }

    CloseHandle(ov.hEvent);
    return bResult;
}

int _tmain(int argc, TCHAR* argv[]) {

    PLACAR_CONTEXT ctx;
    ZeroMemory(&ctx, sizeof(PLACAR_CONTEXT));

    if (!ResolvePipeName(argc, argv, ctx.pipeName, MAX_PIPE_NAME)) {
        return 1;
    }
    _tprintf(_T("NamedPipe = '%s'\n"), ctx.pipeName);

    // Verificar se o Central esta em execucao antes de continuar
    TCHAR pipePath[MAX_PIPE_NAME];
    _stprintf_s(pipePath, MAX_PIPE_NAME, _T("\\\\.\\pipe\\%s"), ctx.pipeName);
    _tprintf(_T("=== Placar Informativo ===\n"));
    ctx.hShutdown = CreateEvent(
        NULL,
        TRUE,      
        FALSE,      // não sinalizado
        NULL
    );
    if (ctx.hShutdown == NULL) {
        _tprintf(_T("Erro ao criar evento de shutdown (erro %lu)\n"), GetLastError());
        return 1;
    }

    ctx.hNotificar = CreateEvent(
        NULL,
        TRUE,                   // manual reset
        FALSE,                  // não sinalizado
        NULL                    // evento anónimo
    );
    if (ctx.hNotificar == NULL) {
        _tprintf(_T("Erro ao criar evento 'notificar' (erro %lu)\n"), GetLastError());
        CloseHandle(ctx.hShutdown);
        return 1;
    }

    ctx.hTimer = CreateWaitableTimer(
        NULL,
        TRUE,       // manual reset
        NULL
    );
    if (ctx.hTimer == NULL) {
        _tprintf(_T("Erro ao criar waitable timer (erro %lu)\n"), GetLastError());
        CloseHandle(ctx.hNotificar);
        CloseHandle(ctx.hShutdown);
        return 1;
    }

    // THREADS (_beginthreadex para evitar memory leaks em runtime)
    HANDLE hThreadUser = (HANDLE)_beginthreadex(
        NULL, 0, UserThread, &ctx, 0, NULL
    );
    if (hThreadUser == NULL) {
        _tprintf(_T("Erro ao criar thread do utilizador (erro %lu)\n"), GetLastError());
        CloseHandle(ctx.hTimer);
        CloseHandle(ctx.hNotificar);
        CloseHandle(ctx.hShutdown);
        return 1;
    }
    ctx.hUserThread = hThreadUser;
    HANDLE hThreadAlert = (HANDLE)_beginthreadex(
        NULL, 0, AlertThread, &ctx, 0, NULL
    );
    if (hThreadAlert == NULL) {
        _tprintf(_T("Erro ao criar thread de alertas (erro %lu)\n"), GetLastError());
        SetEvent(ctx.hShutdown);
        WaitForSingleObject(hThreadUser, 5000);
        CloseHandle(hThreadUser);
        CloseHandle(ctx.hTimer);
        CloseHandle(ctx.hNotificar);
        CloseHandle(ctx.hShutdown);
        return 1;
    }

    _tprintf(_T("Placar iniciado. A aguardar comandos...\n"));

    // Esperar que a thread do utilizador termine
    WaitForSingleObject(hThreadUser, INFINITE);

    // Esperar que a thread de alertas termine
    WaitForSingleObject(hThreadAlert, 5000);

    // Libertar todos os recursos
    CloseHandle(hThreadAlert);
    CloseHandle(hThreadUser);
    CloseHandle(ctx.hTimer);
    CloseHandle(ctx.hNotificar);
    CloseHandle(ctx.hShutdown);

    _tprintf(_T("\nPlacar encerrado com sucesso. Todos os recursos libertados.\n"));
    return 0;
}
