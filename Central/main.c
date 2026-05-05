#include "central.h"

int _tmain(int argc, TCHAR* argv[]) {
	_tprintf(_T("=== Central ===\n"));

	if (argc != 2) {
		_tprintf(_T("Erro: Deve indicar o nome do pipe a utilizar\n"));
		_tprintf(_T("Uso: %s <nome_do_pipe> \n"),argv[0]);
		return 1;
	}

	// garantir que existe apenas uma instância da central de comando em funcionamento
	HANDLE hMutexCentral = CreateMutex(
		NULL, //segurança padrão
		TRUE, //começa trancado
		MUTEX_CENTRAL);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(_T("Erro: já existe uma instância do central.\n"));
		CloseHandle(hMutexCentral);
		return 1;
	}

	CENTRAL_CONTEXT ctx;
	ZeroMemory(&ctx, sizeof(CENTRAL_CONTEXT));
	_tcscpy_s(ctx.pipeName, 256, argv[1]);
	TCHAR buffer[256];
	_stprintf_s(buffer, 256,PIPE_NAME, argv[1]);
	_tcscpy_s(ctx.pipePath, 256, buffer);

	ctx.proximoId = 1;
	ctx.numPlacares = 0;

	InitializeCriticalSection(&ctx.csPlacares);
	ctx.hShutdown = CreateEvent(
		NULL,
		TRUE,      // manual reset: acorda todas as threads de uma vez
		FALSE,      // não sinalizado
		NULL
	);

	if (ctx.hShutdown == NULL) {
		_tprintf(_T("Erro ao criar evento de shutdown (erro %lu)\n"), GetLastError());
		DeleteCriticalSection(&ctx.csPlacares);
		CloseHandle(hMutexCentral);
		return 1;
	}
	
	//lançar thread para aceitar placares
	HANDLE hThreadAccept = (HANDLE)_beginthreadex(
		NULL, 0, AcceptThread, &ctx, 0, NULL
	);
	if (hThreadAccept == NULL) {
		_tprintf(_T("Erro ao criar thread de aceitação de placares (erro %lu)\n"), GetLastError());
		SetEvent(ctx.hShutdown);
		CloseHandle(ctx.hShutdown);
		DeleteCriticalSection(&ctx.csPlacares);
		CloseHandle(hMutexCentral);
		return 1;
	}
	//FALTA: Criar thread de comandos do administrador (areias)


	_tprintf(_T("Central iniciada. ...\n"));


	//FALTA: Esperar que a thread de comandos do administrador termine (areias)

	//Esperar 5s para um placar ser aceite
	WaitForSingleObject(hThreadAccept, 5000);

	// Libertar todos os recursos

	//FALTA: Fechar o handle da thread de comandos do administrador (areias)
	CloseHandle(hThreadAccept);

	// Fechar todos os pipes ainda abertos
	EnterCriticalSection(&ctx.csPlacares);

	for (int i = 0; i < MAX_PLACARES; i++) {
		if (ctx.placares[i].ativo && ctx.placares[i].hPipe != NULL) {
			CloseHandle(ctx.placares[i].hPipe);
		}
	}
	LeaveCriticalSection(&ctx.csPlacares);
	
	CloseHandle(ctx.hShutdown);
	DeleteCriticalSection(&ctx.csPlacares);
	ReleaseMutex(hMutexCentral);
	CloseHandle(hMutexCentral);

	_tprintf(_T("\nCentral encerrada com sucesso. Todos os recursos libertados.\n"));
	return 0;
}