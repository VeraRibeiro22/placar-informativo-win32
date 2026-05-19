#include "central.h"

static BOOL EnviarMensagemPlacar(HANDLE hPipe, LPCVOID buffer, DWORD bufferSize) {
    OVERLAPPED ovW;
    ZeroMemory(&ovW, sizeof(OVERLAPPED));
    ovW.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ovW.hEvent == NULL) return FALSE;

    DWORD bytesWritten = 0;
    // lpNumberOfBytesWritten deve ser NULL em operacoes OVERLAPPED (MSDN)
    BOOL res = WriteFile(hPipe, buffer, bufferSize, NULL, &ovW);
    if (!res && GetLastError() == ERROR_IO_PENDING) {
        WaitForSingleObject(ovW.hEvent, INFINITE);
        res = GetOverlappedResult(hPipe, &ovW, &bytesWritten, FALSE);
    } else if (res) {
        // Completou sincronamente (improvavel em OVERLAPPED mas possivel)
        GetOverlappedResult(hPipe, &ovW, &bytesWritten, FALSE);
    }
    CloseHandle(ovW.hEvent);
    return res;
}

unsigned __stdcall CommandThread(void* param) {
    CENTRAL_CONTEXT* ctx = (CENTRAL_CONTEXT*)param;
    TCHAR command[256];

    _tprintf(_T("\n[CENTRAL] Comandos disponiveis: alerta, cancelar, listar, encerrar\n"));

    while (1) {
        // Verifica se houve shutdown de outra fonte
        if (WaitForSingleObject(ctx->hShutdown, 0) == WAIT_OBJECT_0) {
            break;
        }



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
                    _tprintf(_T("[%d] ID: %lu | Alerta: %s"), 
                        count, 
                        ctx->placares[i].identificador, 
                        ctx->placares[i].temAlerta ? _T("SIM") : _T("NAO"));
                    
                    if (ctx->placares[i].temAlerta) {
                        _tprintf(_T(" ('%s' - %lu segs)"), ctx->placares[i].alertaMsg, ctx->placares[i].alertaDuracao);
                    }
                    _tprintf(_T("\n"));
                }
            }
            if (count == 0) {
                _tprintf(_T("[CENTRAL] Nenhum placar conectado neste momento.\n"));
            }
            _tprintf(_T("-----------------------------\n"));
            LeaveCriticalSection(&ctx->csPlacares);
        }
        // COMANDO ENCERRAR
        else if (_tcscmp(command, _T("encerrar")) == 0) {
            _tprintf(_T("\n [CENTRAL] A encerrar a plataforma central e notificar todos os placares...\n"));
            
            EnterCriticalSection(&ctx->csPlacares);
            MSG_CMD msgEncerrar;
            msgEncerrar.tipo = 6;

            for (int i = 0; i < MAX_PLACARES; i++) {
                if (ctx->placares[i].ativo && ctx->placares[i].hPipe != NULL) {
                    // Envia a mensagem tipo 6 (encerrar plataforma) para o placar
                    if (!EnviarMensagemPlacar(ctx->placares[i].hPipe, &msgEncerrar, sizeof(MSG_CMD))) {
                        _tprintf(_T("[CENTRAL] Erro ao notificar encerramento ao placar ID %lu (erro %lu)\n"), 
                                 ctx->placares[i].identificador, GetLastError());
                    }
                }
            }
            LeaveCriticalSection(&ctx->csPlacares);

            // Avisar todos os monitors que a plataforma vai encerrar
            WaitForSingleObject(ctx->hShmMutex, INFINITE);
            ctx->pShm->desligar = TRUE;
            ReleaseMutex(ctx->hShmMutex);
            SetEvent(ctx->hShmEvent);  // avisar o monitor antes de fechar tudo

            // Sinaliza o evento de shutdown para acordar a AcceptThread e as PlacarHandlerThread
            SetEvent(ctx->hShutdown);
            break;
        }
        // COMANDO ALERTA
        else if (_tcsncmp(command, _T("alerta "), 7) == 0) {
            TCHAR msg[140];
            DWORD duracao = 0;
            DWORD id = 0;
            
            // Syntax: alerta <msg> <duracao> <id>
            BOOL formatoValido = FALSE;
            
            // Encontrar os ultimos dois espacos para extrair id e duracao
            TCHAR* lastSpace = _tcsrchr(command, _T(' '));
            if (lastSpace != NULL) {
                if (_stscanf_s(lastSpace + 1, _T("%lu"), &id) == 1) {
                    *lastSpace = _T('\0');
                    
                    TCHAR* secondLastSpace = _tcsrchr(command, _T(' '));
                    if (secondLastSpace != NULL) {
                        if (_stscanf_s(secondLastSpace + 1, _T("%lu"), &duracao) == 1) {
                            TCHAR* pMsg = command + 7;
                            
                            if (secondLastSpace >= pMsg) {
                                *secondLastSpace = _T('\0');
                                
                                while (*pMsg == _T(' ')) pMsg++;
                                
                                if (_tcslen(pMsg) > 0) {
                                    _tcsncpy_s(msg, 140, pMsg, _TRUNCATE);
                                    formatoValido = TRUE;
                                }
                            }
                        }
                    }
                }
            }
            
            if (formatoValido) {
                MSG_ALERTA msgAlerta;
                msgAlerta.tipo = 4;
                _tcscpy_s(msgAlerta.msg, 140, msg);
                msgAlerta.duracao = duracao;
                DWORD num = 0;
                EnterCriticalSection(&ctx->csPlacares);
                if (id == 0) {
                    // Enviar para todos os placares ativos
                    for (int i = 0; i < MAX_PLACARES; i++) {
                        if (ctx->placares[i].ativo && ctx->placares[i].hPipe != NULL) {
                            EnviarMensagemPlacar(ctx->placares[i].hPipe, &msgAlerta, sizeof(MSG_ALERTA));
                            num++;
                            // atualizar estado local 
                            ctx->placares[i].temAlerta = TRUE;
                            _tcscpy_s(ctx->placares[i].alertaMsg, 140, msg);
                            ctx->placares[i].alertaDuracao = duracao;
                        }
                    }
                    if (num == 0) {
                        _tprintf(_T("[CENTRAL] Nao existem placares ativos.\n"));
                    }
                    else {
                        _tprintf(_T("[CENTRAL] Alerta enviado para todos os placares ativos (%lu).\n"),num);
                    }
                } else {
                    BOOL found = FALSE;
                    for (int i = 0; i < MAX_PLACARES; i++) {
                        if (ctx->placares[i].ativo && ctx->placares[i].identificador == id && ctx->placares[i].hPipe != NULL) {
                            found = TRUE;
                            EnviarMensagemPlacar(ctx->placares[i].hPipe, &msgAlerta, sizeof(MSG_ALERTA));
                            // atualizar estado local
                            ctx->placares[i].temAlerta = TRUE;
                            _tcscpy_s(ctx->placares[i].alertaMsg, 140, msg);
                            ctx->placares[i].alertaDuracao = duracao;
                            _tprintf(_T("[CENTRAL] Alerta enviado ao placar ID %lu.\n"), id);
                            break;
                        }
                    }
                    if (!found) {
                        _tprintf(_T("[CENTRAL] Placar com ID %lu nao encontrado ou inativo.\n"), id);
                    }
                }
                LeaveCriticalSection(&ctx->csPlacares);
            } else {
                _tprintf(_T("[CENTRAL] Formato invalido ou mensagem vazia. Uso: alerta <msg> <duracao> <id>\n"));
            }
        }
        // COMANDO CANCELAR
        else if (_tcsncmp(command, _T("cancelar"), 8) == 0) {
            DWORD id = 0;
            if (_stscanf_s(command, _T("cancelar %lu"), &id) == 1) {
                EnterCriticalSection(&ctx->csPlacares);
                BOOL found = FALSE;
                for (int i = 0; i < MAX_PLACARES; i++) {
                    if (ctx->placares[i].ativo && ctx->placares[i].identificador == id && ctx->placares[i].hPipe != NULL) {
                        found = TRUE;
                        if (!ctx->placares[i].temAlerta) {
                            _tprintf(_T("[CENTRAL] Placar ID %lu nao tem alerta ativo para cancelar.\n"), id);
                            break;
                        }
                        MSG_CMD msgCancelar;
                        msgCancelar.tipo = 5;
                        if (EnviarMensagemPlacar(ctx->placares[i].hPipe, &msgCancelar, sizeof(MSG_CMD))) {
                            _tprintf(_T("[CENTRAL] Comando cancelar enviado ao placar ID %lu.\n"), id);
                            ctx->placares[i].temAlerta = FALSE;
                        } else {
                            _tprintf(_T("[CENTRAL] Erro ao enviar comando cancelar (erro %lu)\n"), GetLastError());
                        }
                        break;
                    }
                }
                LeaveCriticalSection(&ctx->csPlacares);
                if (!found) {
                    _tprintf(_T("[CENTRAL] Placar com ID %lu nao encontrado ou inativo.\n"), id);
                }
            } else {
                _tprintf(_T("[CENTRAL] Formato invalido. Uso: cancelar <id>\n"));
            }
        }
        // OUTROS COMANDOS
        else {
            _tprintf(_T("[CENTRAL] Comando desconhecido: '%s'\n"), command);
            _tprintf(_T("[CENTRAL] Comandos disponiveis: alerta <msg> <dur> <id>, cancelar <id>, listar, encerrar\n"));
        }
    }

    return 0;
}
