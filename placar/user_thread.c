#include "placar.h"
#include <stdlib.h>

unsigned __stdcall UserThread(void* param) {
    PLACAR_CONTEXT* ctx = (PLACAR_CONTEXT*)param;
    TCHAR command[256];

    _tprintf(_T("\nComandos disponiveis: ligar, desligar\n"));

    while (1) {
        _tprintf(_T("> "));

        if (_fgetts(command, 256, stdin) == NULL) {
            break;
        }

        // remover caracteres desnecessarios
        command[_tcscspn(command, _T("\r\n"))] = _T('\0');
        if (_tcslen(command) == 0) {
            continue;
        }

        // COMANDO LIGAR
        if (_tcscmp(command, _T("ligar")) == 0) {
            if (ctx->isConnected) {
                _tprintf(_T("Placar ja esta ligado (Identificador = %lu)\n"), ctx->identificador);
                continue;
            }

            // gerar id aleatorio
            // TODO: na Meta 2 o ID é atribuído pelo central
            srand(GetTickCount());
            ctx->identificador = (DWORD)((rand() % 100) + 1);
            ctx->isConnected = TRUE;

            _tprintf(_T("Identificador = %lu\n"), ctx->identificador);
        }
        // COMANDO DESLIGAR
        else if (_tcscmp(command, _T("desligar")) == 0) {
            if (!ctx->isConnected) {
                _tprintf(_T("Placar nao esta ligado.\n"));
                continue;
            }

            _tprintf(_T("A desligar o placar...\n"));

            // Espera por WaitForMultipleObjects da thread de alertas
            SetEvent(ctx->hShutdown);
            break;
        }
        // COMANDO DESCONHECIDO
        else {
            _tprintf(_T("Comando desconhecido: '%s'\n"), command);
            _tprintf(_T("Comandos disponiveis: ligar, desligar\n"));
        }
    }

    return 0;
}
