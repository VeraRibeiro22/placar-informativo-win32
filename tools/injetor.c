/**
 * injetor.c — Ferramenta auxiliar para testar o placar em M1
 *
 * O que faz:
 *   1. Escreve uma struct MSG_ALERTA no Registry (REG_BINARY)
 *      na chave HKCU\Software\TrabSO2, valor = nome do pipe
 *   2. Abre o evento nomeado "notificar" e sinaliza-o
 *
 * Uso:
 *   injetor.exe <nome_pipe> <mensagem> <duracao_segundos>
 *
 * Exemplo:
 *   injetor.exe tubo "A7 Norte - Acidente Km 34" 30
 *
 * Compilar (linha de comandos MSVC):
 *   cl /W4 /DUNICODE /D_UNICODE injetor.c /link Advapi32.lib
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define REGISTRY_KEY        _T("Software\\TrabSO2")
#define EVENT_NOTIFICAR     _T("notificar")

/* Replica exata da struct do enunciado (igual a ipc_structs.h) */
typedef struct {
    BYTE  tipo;         /* 4 = novo alerta */
    TCHAR msg[140];
    DWORD duracao;
} MSG_ALERTA;

int _tmain(int argc, TCHAR* argv[]) {
    if (argc < 4) {
        _tprintf(_T("Uso: injetor.exe <nome_pipe> <mensagem> <duracao_segundos>\n"));
        _tprintf(_T("Exemplo: injetor.exe tubo \"A7 Norte - Acidente\" 30\n"));
        return 1;
    }

    TCHAR* pipeName = argv[1];
    TCHAR* mensagem = argv[2];
    DWORD  duracao  = (DWORD)_ttoi(argv[3]);

    /* ---------------------------------------------------------------
     * PASSO 1: Construir a struct MSG_ALERTA
     * --------------------------------------------------------------- */
    MSG_ALERTA alerta;
    ZeroMemory(&alerta, sizeof(MSG_ALERTA));

    alerta.tipo   = 4;
    alerta.duracao = duracao;

    /* _tcsncpy_s: cópia segura — trunca se mensagem > 139 TCHAR */
    _tcsncpy_s(alerta.msg, 140, mensagem, _TRUNCATE);

    _tprintf(_T("[injetor] Alerta: '%s' | Duracao: %lu seg\n"), alerta.msg, alerta.duracao);
    _tprintf(_T("[injetor] Tamanho da struct: %zu bytes\n"), sizeof(MSG_ALERTA));

    /* ---------------------------------------------------------------
     * PASSO 2: Escrever no Registry como REG_BINARY
     *
     * Chave:  HKCU\Software\TrabSO2
     * Valor:  <nome_pipe> (ex: "tubo")
     * Tipo:   REG_BINARY
     * Dados:  struct MSG_ALERTA completa (bytes crus)
     * --------------------------------------------------------------- */
    HKEY  hKey;
    LONG  result;

    result = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0, NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL
    );
    if (result != ERROR_SUCCESS) {
        _tprintf(_T("[injetor] Erro ao abrir/criar chave do Registry (%ld)\n"), result);
        return 1;
    }

    result = RegSetValueEx(
        hKey,
        pipeName,               /* Nome do valor = nome do pipe */
        0,
        REG_BINARY,             /* Tipo: bloco de bytes crus */
        (const BYTE*)&alerta,   /* Dados: a struct inteira */
        sizeof(MSG_ALERTA)      /* Tamanho em bytes */
    );
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        _tprintf(_T("[injetor] Erro ao escrever alerta no Registry (%ld)\n"), result);
        return 1;
    }

    _tprintf(_T("[injetor] Alerta escrito no Registry (chave '%s').\n"), pipeName);

    /* ---------------------------------------------------------------
     * PASSO 3: Sinalizar o evento "notificar"
     *
     * OpenEvent — abre um evento nomeado JA EXISTENTE.
     * O placar teve de criar o evento antes (ao arrancar).
     * Se o placar nao estiver a correr, OpenEvent falha.
     * --------------------------------------------------------------- */
    HANDLE hNotificar = OpenEvent(
        EVENT_MODIFY_STATE,     /* Permissao minima necessaria: poder fazer SetEvent */
        FALSE,                  /* Nao herdar o handle */
        EVENT_NOTIFICAR         /* Nome: "notificar" */
    );
    if (hNotificar == NULL) {
        _tprintf(_T("[injetor] ERRO: evento 'notificar' nao encontrado (erro %lu).\n"), GetLastError());
        _tprintf(_T("[injetor] Certifica-te que o placar esta a correr.\n"));
        return 1;
    }

    if (!SetEvent(hNotificar)) {
        _tprintf(_T("[injetor] Erro ao sinalizar evento (erro %lu)\n"), GetLastError());
        CloseHandle(hNotificar);
        return 1;
    }

    CloseHandle(hNotificar);

    _tprintf(_T("[injetor] Evento 'notificar' sinalizado. O placar deve exibir o alerta.\n"));
    return 0;
}
