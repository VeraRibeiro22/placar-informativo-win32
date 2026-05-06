#include "central.h"


unsigned __stdcall AcceptThread(void* param) {
	CENTRAL_CONTEXT* ctx = (CENTRAL_CONTEXT*)param;

	while (1) {
		//verificar shutdown
		if (WaitForSingleObject(ctx->hShutdown, 0) == WAIT_OBJECT_0) {
			break;
		}
		//criar instância do named pipe
		HANDLE hPipe = CreateNamedPipe(
			ctx->pipePath,
			PIPE_ACCESS_DUPLEX,//bidirecional
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, MAX_PLACARES,
			PIPE_BUFFER_SIZE,
			PIPE_BUFFER_SIZE,
			0,
			NULL
		);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(_T("[CENTRAL] Erro ao criar named pipe (erro %lu)\n"), GetLastError());
			break;
		}


		_tprintf(_T("[CENTRAL] A aguardar ligação de placar...\n"));

		//loop fica bloqueado aqui até um placar ligar ou até evento shutdown
		BOOL isConnected = ConnectNamedPipe(hPipe, NULL);
		
		//verificar shutdown
		if (WaitForSingleObject(ctx->hShutdown, 0) == WAIT_OBJECT_0) {
			CloseHandle(hPipe);
			break;
		}

		if (!isConnected && GetLastError() != ERROR_PIPE_CONNECTED) {
			_tprintf(_T("[CENTRAL] Erro ao conectar named pipe (erro %lu)\n"), GetLastError());
			CloseHandle(hPipe);
			continue;
		}

		_tprintf(_T("[CENTRAL] Novo placar conectado.\n"));


		//alocar memória para parâmetro da thread PlacarHandlerThread
		PLACAR_THREAD_DATA* pThreadData = (PLACAR_THREAD_DATA*)malloc(sizeof(PLACAR_THREAD_DATA));

		if (pThreadData == NULL) {
			_tprintf(_T("[CENTRAL] Erro ao alocar memória para parâmetro de thread.\n"));
			CloseHandle(hPipe);
			continue;
		};
		// passar parâmetros para a thread dedicada a um placar (thread dedicada a cada placar)
		pThreadData->ctx = ctx;
		pThreadData->hPipe = hPipe;
		pThreadData->posicao = -1;

		//lançar thread dedicada a um placar
		//HANDLE hThread = (HANDLE)_beginthreadex(
		//	NULL, 0, PlacarHandlerThread, pThreadData, 0, NULL
		//);
		//if (hThread == NULL) {
		//	_tprintf(_T("[CENTRAL] Erro ao criar thread do placar (erro %lu)\n"), GetLastError());
		//	free(pThreadData);
		//	CloseHandle(hPipe);
		//	continue;
		//}
		////fechar o handle da tread uma vez que não é necessário
		//CloseHandle(hThread);
	}

	return 0;
}