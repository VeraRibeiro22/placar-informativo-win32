#include "placar.h"

unsigned __stdcall ListenerThread(void* param) {
    PLACAR_CONTEXT* ctx = (PLACAR_CONTEXT*)param;

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
        // BLOQUEIO: A thread fica aqui a dormir ate a Central enviar algo
        // =================================================================
        if (!ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL) || bytesRead == 0) {
            // Se o ReadFile falhar, significa que o tubo partiu (Central encerrou ou crashou)
            if (ctx->isConnected) {
                _tprintf(_T("\n[Aviso] A ligacao ao Central foi perdida!\n> "));
                ctx->isConnected = FALSE;
                SetEvent(ctx->hShutdown); // Iniciar encerramento do Placar
            }
            break;
        }

        // Pelo protocolo, o primeiro byte de todas as structs e o "tipo"
        BYTE tipo = buffer[0];

        switch (tipo) {
            case 4: // MSG_ALERTA (Novo alerta recebido)
            {
                MSG_ALERTA* alertaRecebido = (MSG_ALERTA*)buffer;
                
                // 1. Guardar a struct no contexto partilhado para a alert_thread
                ctx->alertaAtual = *alertaRecebido;
                
                // 2. Fazer o Eco obrigatrio (enviar a mesma struct de volta)
                DWORD bytesWritten;
                WriteFile(hPipe, alertaRecebido, sizeof(MSG_ALERTA), &bytesWritten, NULL);

                // 3. Tocar a "campainha" para acordar a alert_thread!
                SetEvent(ctx->hNotificar);
                break;
            }
            case 5: // MSG_CMD (Cancelar alerta ativo)
            {
                // 1. Fazer o Eco obrigatorio (enviar MSG_CMD tipo 5 de volta)
                MSG_CMD msgCancelar;
                msgCancelar.tipo = 5;
                DWORD bytesWritten;
                WriteFile(hPipe, &msgCancelar, sizeof(MSG_CMD), &bytesWritten, NULL);

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

    return 0;
}
