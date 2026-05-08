#include "central.h"
unsigned __stdcall PlacarHandlerThread(void* param) {
	PLACAR_THREAD_DATA* tdata = (PLACAR_THREAD_DATA*)param;
	CENTRAL_CONTEXT* ctx = tdata->ctx;
	HANDLE hPipe = tdata->hPipe;

	//Ler pedido de ligar 
	MSG_CMD msgLigar;
	OVERLAPPED ovRead;
	BOOL res;
	DWORD bytesRead, bytesWritten;
	ZeroMemory(&ovRead, sizeof(OVERLAPPED));
	ovRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ovRead.hEvent == NULL) {
		_tprintf(_T("[CENTRAL] Erro ao criar evento de leitura (erro %lu)\n"), GetLastError());
		CloseHandle(hPipe);
		free(tdata);
		return 1;
	}
	res = ReadFile(hPipe, &msgLigar, sizeof(MSG_CMD), NULL, &ovRead);
	if (!res && GetLastError() == ERROR_IO_PENDING) {
		WaitForSingleObject(ovRead.hEvent, INFINITE);
		GetOverlappedResult(hPipe, &ovRead, &bytesRead, FALSE);
	}
	
	//Procurar posicao livre no array de placares
	EnterCriticalSection(&ctx->csPlacares);
	for (int i = 0; i < MAX_PLACARES; i++) {
		if (ctx->placares[i].ativo == FALSE) {
			ctx->placares[i].ativo = TRUE;
			ctx->placares[i].hPipe = hPipe;
			ctx->placares[i].identificador = ctx->proximoId++;
			ctx->placares[i].temAlerta = FALSE;
			ctx->numPlacares++;
			tdata->posicao = i;
			break;
		}
	}
	LeaveCriticalSection(&ctx->csPlacares);
	if (tdata->posicao == -1) {
		_tprintf(_T("[CENTRAL] Numero maximo de placares atingido.\n"));
		CloseHandle(ovRead.hEvent);
		CloseHandle(hPipe);
		free(tdata);
		return 1;
	}

	//Enviar MSG_CMD tipo 7 com o ID
	int pos = tdata->posicao;
	MSG_ID msgId;
	msgId.tipo = 7;
	msgId.identificador = ctx->placares[pos].identificador;

	OVERLAPPED ovWrite;
	ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
	ovWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ovWrite.hEvent == NULL) {
		_tprintf(_T("[CENTRAL] Erro ao criar evento de escrita (erro %lu)\n"), GetLastError());
		CloseHandle(ovRead.hEvent);
		CloseHandle(hPipe);
		free(tdata);
		return 1;
	}
	//Enviar mensagem com o id
	res = WriteFile(hPipe, &msgId, sizeof(MSG_ID), &bytesWritten, &ovWrite);
	if (!res && GetLastError() == ERROR_IO_PENDING) {
		WaitForSingleObject(ovWrite.hEvent, INFINITE);
		GetOverlappedResult(hPipe, &ovWrite, &bytesWritten, FALSE);
	}
	_tprintf(_T("[CENTRAL] Novo placar ligado com ID %lu.\n"), msgId.identificador);

	//LOOP de leitura
	// O placar pode enviar:
	//   MSG_CMD  tipo 2 = desligar       -> enviar eco MSG_CMD tipo 2
	//   MSG_CMD  tipo 3 = fim de alerta  -> sem resposta, atualizar estado
	//   MSG_ALERTA tipo 4 = eco de alerta   
	//   MSG_CMD  tipo 5 = eco de cancelar   

	// Buffer grande o suficiente para acomodar a maior struct (MSG_ALERTA)
	BYTE buffer[1024];
	HANDLE handles[2] = { ovRead.hEvent, ctx->hShutdown };
	BOOL ativo = TRUE;
	while (ativo) {
		ResetEvent(ovRead.hEvent);
		res = ReadFile(hPipe, buffer, sizeof(buffer), NULL, &ovRead);
		if (!res && GetLastError() != ERROR_IO_PENDING) {
			_tprintf(_T("[CENTRAL] Placar com ID %lu desconectado inesperadamente (erro %lu).\n"),
				ctx->placares[pos].identificador, GetLastError());
			break;
		}
		DWORD waitResult = WaitForMultipleObjects(2,handles,FALSE,INFINITE);
		if (waitResult == WAIT_OBJECT_0) {
			if (!GetOverlappedResult(hPipe, &ovRead, &bytesRead, FALSE) || bytesRead == 0) {
				_tprintf(_T("[CENTRAL] Placar com ID %lu desconectado inesperadamente (erro %lu).\n"),ctx->placares[pos].identificador, GetLastError());
				break;
			}
			// Pelo protocolo, o primeiro byte de todas as structs e o "tipo"
			BYTE tipo = buffer[0];
			switch (tipo) {
			case 2:
			{ //   MSG_CMD  tipo 2 = desligar       -> enviar eco MSG_CMD tipo 2
				MSG_CMD* msg_desligar = (MSG_CMD*)buffer;
				ResetEvent(ovWrite.hEvent);

				//Enviar eco para o placar
				res = WriteFile(hPipe, msg_desligar, sizeof(MSG_CMD), &bytesWritten, &ovWrite);
				if (!res && GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(ovWrite.hEvent, INFINITE);
					GetOverlappedResult(hPipe, &ovWrite, &bytesWritten, FALSE);
				}
				_tprintf(_T("[CENTRAL] Placar com ID %lu desligado.\n"), ctx->placares[pos].identificador);
				ativo = FALSE;
				break;
			}
			case 3:
			{ //   MSG_CMD  tipo 3 = fim de alerta    -> atualizar estado
				EnterCriticalSection(&ctx->csPlacares);
				ctx->placares[pos].temAlerta = FALSE;
				LeaveCriticalSection(&ctx->csPlacares);
				_tprintf(_T("[CENTRAL] Alerta do placar com ID %lu terminou.\n"), ctx->placares[pos].identificador);
				break;
			}
			case 4:
			{ //   MSG_CMD  tipo 4 = eco do alerta    -> atualizar estado
				MSG_ALERTA* msg_alerta_eco = (MSG_ALERTA*)buffer;
				EnterCriticalSection(&ctx->csPlacares);
				ctx->placares[pos].temAlerta = TRUE;
				LeaveCriticalSection(&ctx->csPlacares);
				_tprintf(_T("[CENTRAL] Placar com ID %lu confirmou receção de alerta '%s'.\n"), ctx->placares[pos].identificador, msg_alerta_eco->msg);
				break;
			}
			case 5: // MSG_CMD  tipo 5 = eco do cancelar
			{
				EnterCriticalSection(&ctx->csPlacares);
				ctx->placares[pos].temAlerta = FALSE;
				LeaveCriticalSection(&ctx->csPlacares);
				_tprintf(_T("[CENTRAL] Placar com ID %lu confirmou cancelamento de alerta.\n"), ctx->placares[pos].identificador);
				break;
			}
			default:
			{
				_tprintf(_T("[CENTRAL] Placar com ID %lu enviou mensagem de tipo desconhecido.\n"), ctx->placares[pos].identificador);
				break;
			}
			}
		}
		else if (waitResult == WAIT_OBJECT_0 + 1) { // evento de shutdown
			CancelIo(hPipe);
			GetOverlappedResult(hPipe, &ovRead, &bytesRead, TRUE);
			break;
		}
	
	}


	//Limpar recursos
	EnterCriticalSection(&ctx->csPlacares);
	ctx->placares[pos].ativo = FALSE;
	ctx->placares[pos].hPipe = NULL;
	ctx->placares[pos].temAlerta = FALSE;
	ctx->numPlacares--;
	LeaveCriticalSection(&ctx->csPlacares);
	CloseHandle(ovRead.hEvent);
	CloseHandle(ovWrite.hEvent);
	CloseHandle(hPipe);
	free(tdata);
	return 0;
}

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
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
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
		HANDLE hThread = (HANDLE)_beginthreadex(
			NULL, 0, PlacarHandlerThread, pThreadData, 0, NULL
		);
		if (hThread == NULL) {
			_tprintf(_T("[CENTRAL] Erro ao criar thread do placar (erro %lu)\n"), GetLastError());
			free(pThreadData);
			CloseHandle(hPipe);
			continue;
		}
		//fechar o handle da tread uma vez que não é necessário
		CloseHandle(hThread);
	}

	return 0;
}