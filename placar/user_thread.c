#include "placar.h"

unsigned __stdcall UserThread(void* param) {
    PLACAR_CONTEXT* ctx = (PLACAR_CONTEXT*)param;
    TCHAR command[64];

    _tprintf(_T("Comandos disponiveis: ligar, desligar\n"));

    while (1) {
        _tprintf(_T("> "));
        if (_fgetts(command, 64, stdin) == NULL) {
            break;
        }
        command[_tcscspn(command, _T("\r\n"))] = _T('\0');

        // COMANDO: ligar
        if (_tcscmp(command, _T("ligar")) == 0) {
            if (ctx->isConnected) {
                _tprintf(_T("Erro: placar ja esta ligado.\n"));
                continue;
            }

            TCHAR pipePath[MAX_PIPE_NAME];
            _stprintf_s(pipePath, MAX_PIPE_NAME, _T("\\\\.\\pipe\\%s"), ctx->pipeName);

            HANDLE hPipe = INVALID_HANDLE_VALUE;
            int tentativa;
            for (tentativa = 0; tentativa < 5; tentativa++) {
                hPipe = CreateFile(
                    pipePath,             
                    GENERIC_READ | GENERIC_WRITE, 
                    0,                    
                    NULL,                 
                    OPEN_EXISTING,        
                    0,                    
                    NULL
                );
                if (hPipe != INVALID_HANDLE_VALUE) break;

                if (GetLastError() != ERROR_PIPE_BUSY) {
                    _tprintf(_T("Erro: nao foi possivel ligar ao central (erro %lu)\n"), GetLastError());
                    break;
                }
                WaitNamedPipe(pipePath, 1000); // esperar ate 1s antes de tentar de novo
            }

            if (hPipe == INVALID_HANDLE_VALUE) {
                _tprintf(_T("Erro: falha definitiva ao ligar ao central.\n"));
                continue;
            }

            // Configurar o modo de leitura do pipe como MESSAGE 
            DWORD dwMode = PIPE_READMODE_MESSAGE;
            if (!SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL)) {
                _tprintf(_T("Aviso: SetNamedPipeHandleState falhou (erro %lu)\n"), GetLastError());
            }

            ctx->hPipe = hPipe;

            // Enviar pedido de ligacao ao central: MSG_CMD tipo 1
            MSG_CMD msgLigar;
            msgLigar.tipo = 1;
            DWORD bytesWritten;
            if (!WriteFile(hPipe, &msgLigar, sizeof(MSG_CMD), &bytesWritten, NULL)) {
                _tprintf(_T("Erro ao enviar pedido de ligacao (erro %lu)\n"), GetLastError());
                CloseHandle(hPipe);
                ctx->hPipe = NULL;
                continue;
            }

            // Aguardar resposta do central: MSG_ID (tipo 7) com o identificador atribuido
            MSG_ID msgId;
            DWORD bytesRead;
            if (!ReadFile(hPipe, &msgId, sizeof(MSG_ID), &bytesRead, NULL)) {
                _tprintf(_T("Erro ao receber identificador do central (erro %lu)\n"), GetLastError());
                CloseHandle(hPipe);
                ctx->hPipe = NULL;
                continue;
            }

            if (msgId.tipo != 7) {
                _tprintf(_T("Erro: resposta inesperada do central (tipo=%d)\n"), msgId.tipo);
                CloseHandle(hPipe);
                ctx->hPipe = NULL;
                continue;
            }

            ctx->identificador = msgId.identificador;
            ctx->isConnected = TRUE;
            _tprintf(_T("Identificador = %lu\n"), ctx->identificador);
        }
        // COMANDO: desligar
        // Envia MSG_CMD tipo 2 ao central e termina o placar.
        else if (_tcscmp(command, _T("desligar")) == 0) {
            if (ctx->isConnected && ctx->hPipe != NULL) {
                MSG_CMD msgDesligar;
                msgDesligar.tipo = 2;
                DWORD bytesWritten;
                WriteFile(ctx->hPipe, &msgDesligar, sizeof(MSG_CMD), &bytesWritten, NULL);

                CloseHandle(ctx->hPipe);
                ctx->hPipe = NULL;
            }
            SetEvent(ctx->hShutdown);
            break;
        }
        else {
            _tprintf(_T("Comando desconhecido: '%s'\n"), command);
            _tprintf(_T("Comandos disponiveis: ligar, desligar\n"));
        }
    }

    return 0;
}
