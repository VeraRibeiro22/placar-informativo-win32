#include "central.h"

unsigned __stdcall CommandThread(void* param) {
    CENTRAL_CONTEXT* ctx = (CENTRAL_CONTEXT*)param;
    TCHAR command[256];

    _tprintf(_T("\nComandos disponiveis: alerta, cancelar, listar, encerrar\n"));

    while (1) {
        // Verifica se houve shutdown de outra fonte
        if (WaitForSingleObject(ctx->hShutdown, 0) == WAIT_OBJECT_0) {
            break;
        }

        _tprintf(_T("Central> "));

        if (_fgetts(command, 256, stdin) == NULL) {
            break;
        }

        // remover caracteres desnecessarios (newline e carriage return)
        command[_tcscspn(command, _T("\r\n"))] = _T('\0');
        if (_tcslen(command) == 0) {
            continue;
        }

        // COMANDO LISTAR
        if (_tcscmp(command, _T("listar")) == 0) {
            EnterCriticalSection(&ctx->csPlacares);
            _tprintf(_T("\n--- Placares Ativos (%lu) ---\n"), ctx->numPlacares);
            
            int count = 0;
            for (int i = 0; i < MAX_PLACARES; i++) {
                if (ctx->placares[i].ativo) {
                    count++;
                    _tprintf(_T("[%d] ID: %lu | Pipe: %p | Alerta: %s"), 
                        count, 
                        ctx->placares[i].identificador, 
                        ctx->placares[i].hPipe,
                        ctx->placares[i].temAlerta ? _T("SIM") : _T("NAO"));
                    
                    if (ctx->placares[i].temAlerta) {
                        _tprintf(_T(" ('%s' - %lu segs)"), ctx->placares[i].alertaMsg, ctx->placares[i].alertaDuracao);
                    }
                    _tprintf(_T("\n"));
                }
            }
            if (count == 0) {
                _tprintf(_T("Nenhum placar conectado neste momento.\n"));
            }
            _tprintf(_T("-----------------------------\n"));
            LeaveCriticalSection(&ctx->csPlacares);
        }
        // COMANDO ENCERRAR
        else if (_tcscmp(command, _T("encerrar")) == 0) {
            _tprintf(_T("\nA encerrar a plataforma central e notificar todos os placares...\n"));
            
            EnterCriticalSection(&ctx->csPlacares);
            MSG_CMD msgEncerrar;
            msgEncerrar.tipo = 6;
            DWORD bytesWritten;

            for (int i = 0; i < MAX_PLACARES; i++) {
                if (ctx->placares[i].ativo && ctx->placares[i].hPipe != NULL) {
                    // Envia a mensagem tipo 6 (encerrar plataforma) para o placar
                    if (!WriteFile(ctx->placares[i].hPipe, &msgEncerrar, sizeof(MSG_CMD), &bytesWritten, NULL)) {
                        _tprintf(_T("Erro ao notificar encerramento ao placar ID %lu (erro %lu)\n"), 
                                 ctx->placares[i].identificador, GetLastError());
                    }
                }
            }
            LeaveCriticalSection(&ctx->csPlacares);

            // Sinaliza o evento de shutdown para acordar a AcceptThread e as PlacarHandlerThread
            SetEvent(ctx->hShutdown);
            break;
        }
        // OUTROS COMANDOS
        else {
            // Verificar de forma simples se comeca com os comandos que vamos implementar depois
            if (_tcsncmp(command, _T("alerta"), 6) == 0 || _tcsncmp(command, _T("cancelar"), 8) == 0) {
                _tprintf(_T("Comando '%s' ainda em implementacao (proxima etapa).\n"), command);
            } else {
                _tprintf(_T("Comando desconhecido: '%s'\n"), command);
                _tprintf(_T("Comandos disponiveis: alerta, cancelar, listar, encerrar\n"));
            }
        }
    }

    return 0;
}
