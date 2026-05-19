#include "monitor.h"


int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int nCmdShow) {
	MONITOR_CONTEXT ctx;
	ZeroMemory(&ctx, sizeof(MONITOR_CONTEXT));
	
	ctx.maxAlertas = MAX_ALERTAS_DEFAULT;
	_tcscpy_s(ctx.nomeShm, 256, _T("Placar_Shared_Memory"));
	_tcscpy_s(ctx.nomeMutex, 256, _T("Placar_Mutex_SHM"));
	_tcscpy_s(ctx.nomeEvento, 256, _T("Placar_Event_Update"));

	// evento de shutdown para a ShmThread (anonimo e manual-reset)
	ctx.hShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ctx.hShutdown == NULL) {
		MessageBox(NULL, _T("Erro ao criar evento de shutdown."),
			_T("Erro Crítico"), MB_OK | MB_ICONERROR);
		return 1;
	}


	WNDCLASSEX wcApp;
	MSG lpMsg;

	// Definição das características da janela "wcApp" 
	wcApp.cbSize = sizeof(WNDCLASSEX);
	wcApp.hInstance = hInst;
	wcApp.lpszClassName = SZPROGNAME;
	wcApp.lpfnWndProc = WndProc;
		wcApp.style = CS_HREDRAW | CS_VREDRAW;  // Estilo da janela: Fazer o redraw se for modificada horizontal ou verticalmente
	wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcApp.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcApp.cbClsExtra = 0;
	wcApp.cbWndExtra = sizeof(MONITOR_CONTEXT*);;
	wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcApp.lpszMenuName = MAKEINTRESOURCE(IDC_MONITOR);
	if (!RegisterClassEx(&wcApp)) {
			MessageBox(NULL, _T("Erro ao registar classe de janela."),_T("Erro Crítico"), MB_OK | MB_ICONERROR);
			CloseHandle(ctx.hShutdown);
			return 1;
	}
	//Criar janela
	ctx.hWnd = CreateWindow(
		SZPROGNAME,
		TEXT("PROGRAMA MONITOR"),
		WS_OVERLAPPEDWINDOW,	// Estilo da janela 
		CW_USEDEFAULT,  // Posição x pixels (default - à direita da última)
		CW_USEDEFAULT,  // Posição y pixels (default - abaixo da última)
		CW_USEDEFAULT,  // Largura da janela (em pixels)
		CW_USEDEFAULT,  // Altura da janela (em pixels)
		(HWND)HWND_DESKTOP,	// handle da janela pai (se se criar uma a partir de outra) ou HWND_DESKTOP se a janela for a primeira, criada a partir do "desktop"
		(HMENU)NULL,  // handle do menu da janela 
		(HINSTANCE)hInst,  // handle da instância do programa actual
		NULL);
	if (ctx.hWnd == NULL) {
		MessageBox(NULL, _T("Erro ao criar janela principal."),_T("Erro Crítico"), MB_OK | MB_ICONERROR);
		CloseHandle(ctx.hShutdown);
		return 1;
	}
	SetWindowLongPtr(ctx.hWnd, 0, (LONG_PTR)&ctx);
	// Mostrar a janela
	ShowWindow(ctx.hWnd, nCmdShow);
	UpdateWindow(ctx.hWnd);

	//loop de mensagens
	while (GetMessage(&lpMsg, NULL, 0, 0) > 0) {
		TranslateMessage(&lpMsg);  // Pré-processamento da mensagem 
		DispatchMessage(&lpMsg);  // Enviar a mensagem traduzida de volta ao Windows, que aguarda até que a possa reenviar à função de tratamento da janela
	}
	// sinalizar e aguardar a ShmThread 
	SetEvent(ctx.hShutdown);
	if (ctx.hShmThread != NULL) {
		WaitForSingleObject(ctx.hShmThread, INFINITE);
		CloseHandle(ctx.hShmThread);
	}

	// Fechar handles se estiverem abertos
	if (ctx.hShm)   CloseHandle(ctx.hShm);
	if (ctx.hMutex) CloseHandle(ctx.hMutex);
	if (ctx.hEvent) CloseHandle(ctx.hEvent);

	CloseHandle(ctx.hShutdown);
	//encerrar o programa
	return (int)lpMsg.wParam;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	MONITOR_CONTEXT* pCtx = (MONITOR_CONTEXT*)GetWindowLongPtr(hWnd, 0);
	switch (messg) {
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		if (pCtx != NULL) {
			//acabar implementacao
		}

		EndPaint(hWnd, &ps);
		break;
	}
	case WM_COMMAND: {

		//clicar em configuracao
		if (wParam == ID_FICHEIRO_CONFIG) {
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_EDIT_MAX), hWnd, (DLGPROC)ConfigDlgProc, (LPARAM)pCtx);
		}
		else if (wParam == ID_FICHEIRO_ACERCA) {
			MessageBox(hWnd,
				_T("Trabalho realizado por:\n- Pedro Amorim (2022157609) | Vera Ribeiro (2024140140)"),
				_T("Acerca"),
				MB_OK | MB_ICONINFORMATION);
		}
		else if (wParam == ID_FICHEIRO_SAIR) {
			if (pCtx != NULL && pCtx->hShutdown != NULL) {
				SetEvent(pCtx->hShutdown);
			}
			PostQuitMessage(0); // Fecha a janela
		}
		break;
	}
	case WM_DESTROY:
	{
		if (pCtx != NULL && pCtx->hShutdown != NULL) {
			SetEvent(pCtx->hShutdown);
		}
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hWnd, messg, wParam, lParam);
	}
	return 0;
}
LRESULT CALLBACK ConfigDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam) {
	MONITOR_CONTEXT* pCtx;

	switch (messg) {
		case WM_INITDIALOG:
		{
			//receber o pCtx através do lParam e guardamos na memória da janela
			pCtx = (MONITOR_CONTEXT*)lParam;
			SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pCtx);
			// preencher as caixas de texto com os valores que já estão no pCtx
			SetDlgItemInt(hDlg, IDC_EDIT_MAX, pCtx->maxAlertas, FALSE);
			SetDlgItemText(hDlg, IDC_EDIT_SHM, pCtx->nomeShm);
			SetDlgItemText(hDlg, IDC_EDIT_MUTEX, pCtx->nomeMutex);
			SetDlgItemText(hDlg, IDC_EDIT_EVENTO, pCtx->nomeEvento);

			return TRUE;
		}
		case WM_COMMAND:
		{
			//clicar em OK
			if (wParam == IDOK) {
				//recuperar o ponteiro guardado
				pCtx = (MONITOR_CONTEXT*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
				if (pCtx != NULL) {
					// ler o que foi escrito nas caixas de texto e guardar de volta no contexto
					pCtx->maxAlertas = GetDlgItemInt(hDlg, IDC_EDIT_MAX, NULL, FALSE);
					GetDlgItemText(hDlg, IDC_EDIT_SHM, pCtx->nomeShm, 256);
					GetDlgItemText(hDlg, IDC_EDIT_MUTEX, pCtx->nomeMutex, 256);
					GetDlgItemText(hDlg, IDC_EDIT_EVENTO, pCtx->nomeEvento, 256);
				}
				EndDialog(hDlg, IDOK);
				return TRUE;
			}
			//clicar em cancelar
			else if (wParam == IDCANCEL) {
				EndDialog(hDlg, IDCANCEL);
				return TRUE;
			}
			break;
		}
		default:
			return FALSE;
	}
	return FALSE;
}