#include "placar.h"

void FormatTimestamp(TCHAR* buffer, DWORD size) {
    SYSTEMTIME st;

    GetLocalTime(&st);

    _sntprintf_s(buffer, size, _TRUNCATE,
        _T("%02d/%02d/%04d (%02d:%02d:%02d)"),
        st.wDay, st.wMonth, st.wYear,
        st.wHour, st.wMinute, st.wSecond);
}

static BOOL ConfigureTimer(HANDLE hTimer, DWORD durationSeconds) {
    LARGE_INTEGER dueTime;

    /*
     * Tempo relativo em unidades de 100ns, negativo por convenção Win32:
     *   - Positivo = tempo absoluto (FILETIME)
     *   - Negativo = tempo relativo a partir de agora
     *
     * 1 segundo = 10.000.000 unidades de 100ns
     */
    dueTime.QuadPart = -(LONGLONG)durationSeconds * 10000000LL;

    if (!SetWaitableTimer(hTimer, &dueTime, 0, NULL, NULL, FALSE)) {
        _tprintf(_T("Erro ao configurar waitable timer (erro %lu)\n"), GetLastError());
        return FALSE;
    }

    return TRUE;
}

unsigned __stdcall AlertThread(void* param) {
    PLACAR_CONTEXT* ctx = (PLACAR_CONTEXT*)param;
    TCHAR timestamp[64];
    MSG_ALERTA alerta;

    // Outer: [0]=hNotificar, [1]=hShutdown
    // Inner: [0]=hTimer, [1]=hNotificar, [2]=hShutdown
    HANDLE outerHandles[2] = { ctx->hNotificar, ctx->hShutdown };
    HANDLE innerHandles[3] = { ctx->hTimer, ctx->hNotificar, ctx->hShutdown };

    // Outer Loop: espera por alerta ou shutdown
    while (1) {
        DWORD waitResult = WaitForMultipleObjects(2, outerHandles, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0) { // Notificar
            ResetEvent(ctx->hNotificar);

            if (!ctx->isConnected) { //se nao estiver ligado, ignora o alerta
                continue;
            }

            alerta = ctx->alertaAtual;

            FormatTimestamp(timestamp, 64);
            _tprintf(_T("\n%s: '%s'\n> "), timestamp, alerta.msg);

            fflush(stdout);

            if (!ConfigureTimer(ctx->hTimer, alerta.duracao)) {
                continue;
            }

            // Inner Loop: espera por timer, novo alerta, ou shutdown
            BOOL alertActive = TRUE;
            while (alertActive) {
                DWORD innerResult = WaitForMultipleObjects(3, innerHandles, FALSE, INFINITE);

                if (innerResult == WAIT_OBJECT_0) { // Timer
                    FormatTimestamp(timestamp, 64);
                    _tprintf(_T("\n%s: '---'\n> "), timestamp);

                    // Notificar o Central que o alerta expirou (tipo 3)
                    if (ctx->hPipe != NULL) {
                        MSG_CMD msgFim;
                        msgFim.tipo = 3;
                        EnviarMensagemCentral(ctx->hPipe, &msgFim, sizeof(MSG_CMD));
                    }

                    fflush(stdout);

                    alertActive = FALSE;
                }
                else if (innerResult == WAIT_OBJECT_0 + 1) { // Notificar
                    ResetEvent(ctx->hNotificar);

                    CancelWaitableTimer(ctx->hTimer); // remover timer anterior

                    alerta = ctx->alertaAtual;

                    FormatTimestamp(timestamp, 64);
                    _tprintf(_T("\n%s: '%s'\n> "), timestamp, alerta.msg);
                    fflush(stdout);
                    if (!ConfigureTimer(ctx->hTimer, alerta.duracao)) {
                        alertActive = FALSE;
                    }
                }
                else if (innerResult == WAIT_OBJECT_0 + 2) { // Shutdown
                    CancelWaitableTimer(ctx->hTimer);
                    return 0;
                }
                else { // Erro inesperado
                    _tprintf(_T("Erro em WaitForMultipleObjects interno (erro %lu)\n"), GetLastError());
                    return 1;
                }
            }
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) { // Shutdown
            return 0;
        }
        else { // Erro inesperado
            _tprintf(_T("Erro em WaitForMultipleObjects externo (erro %lu)\n"), GetLastError());
            return 1;
        }
    }

    return 0;
}
