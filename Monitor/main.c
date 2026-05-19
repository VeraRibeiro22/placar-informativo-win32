#include "monitor.h"


int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int nCmdShow) {
	MONITOR_CONTEXT ctx;
	ZeroMemory(&ctx, sizeof(MONITOR_CONTEXT));
	
	ctx.maxAlertas = MAX_ALERTAS_DEFAULT;
	ctx.numAlertaAtivos = 0;
	ctx.offset = 0;
	_tcscpy_s(ctx.nomeShm, 256, _T("Placar_Shared_Memory"));
	_tcscpy_s(ctx.nomeMutex, 256, _T("Placar_Mutex_SHM"));
	_tcscpy_s(ctx.nomeEvento, 256, _T("Placar_Event_Update"));

	// evento de shutdown para a ShmThread (anonimo e manual-reset)
	ctx.hShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ctx.hShutdown == NULL) {
		MessageBox(NULL, _T("Erro ao criar evento de shutdown."),
			_T("Erro Cr?tico"), MB_OK | MB_ICONERROR);
		return 1;
	}


	WNDCLASSEX wcApp;
	MSG lpMsg;

	// Defini??o das caracter?sticas da janela "wcApp" 
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
			MessageBox(NULL, _T("Erro ao registar classe de janela."),_T("Erro Cr?tico"), MB_OK | MB_ICONERROR);
			CloseHandle(ctx.hShutdown);
			return 1;
	}
	//Criar janela
	ctx.hWnd = CreateWindow(
		SZPROGNAME,
		TEXT("PROGRAMA MONITOR"),
		WS_OVERLAPPEDWINDOW,	// Estilo da janela 
		CW_USEDEFAULT,  // Posi??o x pixels (default - ? direita da ?ltima)
		CW_USEDEFAULT,  // Posi??o y pixels (default - abaixo da ?ltima)
		CW_USEDEFAULT,  // Largura da janela (em pixels)
		CW_USEDEFAULT,  // Altura da janela (em pixels)
		(HWND)HWND_DESKTOP,	// handle da janela pai (se se criar uma a partir de outra) ou HWND_DESKTOP se a janela for a primeira, criada a partir do "desktop"
		(HMENU)NULL,  // handle do menu da janela 
		(HINSTANCE)hInst,  // handle da inst?ncia do programa actual
		NULL);
	if (ctx.hWnd == NULL) {
		MessageBox(NULL, _T("Erro ao criar janela principal."),_T("Erro Cr?tico"), MB_OK | MB_ICONERROR);
		CloseHandle(ctx.hShutdown);
		return 1;
	}
	SetWindowLongPtr(ctx.hWnd, 0, (LONG_PTR)&ctx);

	// Mostrar a janela
	ShowWindow(ctx.hWnd, nCmdShow);
	UpdateWindow(ctx.hWnd);

	// Lan?a a thread de conex?o 
	ctx.hConnectThread = (HANDLE)_beginthreadex(NULL, 0, ConnectThread, &ctx, 0, NULL);

	//loop de mensagens
	while (GetMessage(&lpMsg, NULL, 0, 0) > 0) {
		TranslateMessage(&lpMsg);  // Pr?-processamento da mensagem 
		DispatchMessage(&lpMsg);  // Enviar a mensagem traduzida de volta ao Windows, que aguarda at? que a possa reenviar ? fun??o de tratamento da janela
	}

	// Cleanup
	// sinalizar e aguardar a ShmThread 
	SetEvent(ctx.hShutdown);
	if (ctx.hConnectThread != NULL) {
		WaitForSingleObject(ctx.hConnectThread, 3000);
		CloseHandle(ctx.hConnectThread);
	}
	if (ctx.hShmThread != NULL) {
		WaitForSingleObject(ctx.hShmThread, INFINITE);
		CloseHandle(ctx.hShmThread);
	}
	if (ctx.pShm)   UnmapViewOfFile(ctx.pShm);
	if (ctx.hShm)   CloseHandle(ctx.hShm);
	if (ctx.hMutex) CloseHandle(ctx.hMutex);
	if (ctx.hEvent) CloseHandle(ctx.hEvent);
	CloseHandle(ctx.hShutdown);
	return (int)lpMsg.wParam;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	MONITOR_CONTEXT* pCtx = (MONITOR_CONTEXT*)GetWindowLongPtr(hWnd, 0);
	switch (messg) {
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		if (!pCtx->centralLigada) {
			TextOut(hdc, 10, 10, _T("====== Central indisponivel ======"), 34);

		} else if (pCtx != NULL && pCtx->numAlertaAtivos == 0) {
			TextOut(hdc, 10, 10, _T("====== Sem alertas ativos ======"), 33);
		}
		else if (pCtx != NULL) {
			TCHAR str[300];
			for (int i = pCtx->offset, linha = 0; i < pCtx->numAlertaAtivos && linha < pCtx->maxAlertas; i++, linha++) {
				_stprintf_s(str, 300, _T("ID PLACAR: %lu |MENSAGEM DE ALERTA: %s | DURAÇÃO: %lu s"),
					pCtx->alertasLocais[i].identificador,
					pCtx->alertasLocais[i].msg,
					pCtx->alertasLocais[i].duracao);
				TextOut(hdc, 10, 10 + linha * 20, str, (int)_tcslen(str));
			}
		}

		EndPaint(hWnd, &ps);
		break;
	}
	case WM_COMMAND: {

		//clicar em configuracao
		if (wParam == ID_FICHEIRO_CONFIG) {
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_EDIT_MAX), hWnd, (DLGPROC)ConfigDlgProc, (LPARAM)pCtx);
			if (pCtx != NULL) {
				pCtx->offset = 0;
				InvalidateRect(hWnd, NULL, TRUE);
			}
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
	case WM_KEYDOWN:
	{
		if (wParam == VK_NEXT) { // Page Down
			if (pCtx != NULL && pCtx->offset + pCtx->maxAlertas < pCtx->numAlertaAtivos) {
				pCtx->offset += pCtx->maxAlertas;
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
		else if (wParam == VK_PRIOR) { // Page Up
			if (pCtx != NULL && pCtx->offset > 0) {
				pCtx->offset -= pCtx->maxAlertas;
				if (pCtx->offset < 0) pCtx->offset = 0;
				InvalidateRect(hWnd, NULL, TRUE);
			}
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
			//receber o pCtx atrav?s do lParam e guardamos na mem?ria da janela
			pCtx = (MONITOR_CONTEXT*)lParam;
			SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pCtx);
			// preencher as caixas de texto com os valores que j? est?o no pCtx
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
