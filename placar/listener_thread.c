#include "placar.h"

unsigned __stdcall ListenerThread(void* param) {
    PLACAR_CONTEXT* ctx = (PLACAR_CONTEXT*)param;

    OVERLAPPED ov;
    ZeroMemory(&ov, sizeof(OVERLAPPED));
    ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ov.hEvent == NULL) {
        _tprintf(_T("\n[Placar] Erro ao criar evento OVERLAPPED na ListenerThread.\n"));
        return 1;
    }

    HANDLE waitHandles[2] = { ov.hEvent, ctx->hShutdown };

    while (1) {
        HANDLE hPipe = ctx->hPipe;
        if (hPipe == NULL) {
            // Se o pipe foi fechado localmente (p. ex: pelo comando desligar)
            break;
        }

        // Buffer grande o suficiente para acomodar a maior struct (MSG_ALERTA)
        BYTE buffer[1024];
        DWORD bytesRead;

        // =================================================================
        // Leitura Assincrona: A thread espera por dados da Central ou shutdown
        // =================================================================
        BOOL bRead = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, &ov);
        DWORD err = GetLastError();

        if (!bRead && err == ERROR_IO_PENDING) {
            DWORD waitRes = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
            if (waitRes == WAIT_OBJECT_0) {
                bRead = GetOverlappedResult(hPipe, &ov, &bytesRead, FALSE);
                ResetEvent(ov.hEvent);
            } else if (waitRes == WAIT_OBJECT_0 + 1) {
                // Comando de shutdown recebido enquanto esperava leitura
                CancelIo(hPipe);
                break;
            } else {
                CancelIo(hPipe);
                break; // Erro no WaitForMultipleObjects
            }
        }

        if (!bRead || bytesRead == 0) {
            // Pipe quebrou: Central encerrou, ou o placar fez desligar e a Central fechou o pipe.
            // Garantir que AlertThread acorda e termina (hShutdown auto-reset).
            _tprintf(_T("\n[Placar] Ligacao ao Central terminada.\n"));
            ctx->isConnected = FALSE;
            SetEvent(ctx->hShutdown);
            break;
        }

        // Pelo protocolo, o primeiro byte de todas as structs e o "tipo"
        BYTE tipo = buffer[0];

        switch (tipo) {
            case 2: // MSG_CMD tipo 2: eco do desligar (pode chegar antes do pipe fechar)
            {      // Ignorar — o placar ja encerrou de forma fire-and-forget
                break;
            }
            case 4: // MSG_ALERTA (Novo alerta recebido)
            {
                MSG_ALERTA* alertaRecebido = (MSG_ALERTA*)buffer;
                
                // 1. Guardar a struct no contexto partilhado para a alert_thread
                ctx->alertaAtual = *alertaRecebido;
                
                // 2. Fazer o Eco obrigatorio (enviar a mesma struct de volta)
                EnviarMensagemCentral(hPipe, alertaRecebido, sizeof(MSG_ALERTA));

                // 3. Tocar a "campainha" para acordar a alert_thread!
                SetEvent(ctx->hNotificar);
                break;
            }
            case 5: // MSG_CMD (Cancelar alerta ativo)
            {
                // 1. Fazer o Eco obrigatorio (enviar MSG_CMD tipo 5 de volta)
                MSG_CMD msgCancelar;
                msgCancelar.tipo = 5;
                EnviarMensagemCentral(hPipe, &msgCancelar, sizeof(MSG_CMD));

                // 2. Limpar a mensagem atual
                ZeroMemory(&ctx->alertaAtual, sizeof(MSG_ALERTA));

                // 3. Parar o Waitable Timer imediatamente
                // O Win32 permite cancelar timers a partir de qualquer thread!
                CancelWaitableTimer(ctx->hTimer);
                
                // 4. Mostrar o "---" na consola conforme exigido (sem hora)
                _tprintf(_T("\n---\n> "));
                break;
            }
            case 6: // MSG_CMD (Encerrar plataforma)
            {
                _tprintf(_T("\n[CENTRAL] Ordem de encerramento geral recebida. A desligar...\n"));
                SetEvent(ctx->hShutdown);
                break;
            }
            default:
                _tprintf(_T("\n[Aviso] Mensagem desconhecida recebida do Central (tipo %d)\n> "), tipo);
                break;
        }
    }

    CloseHandle(ov.hEvent);
    return 0;
}
