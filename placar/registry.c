#include "placar.h"

BOOL ResolvePipeName(int argc, TCHAR* argv[], TCHAR* outPipeName, DWORD size) {
    HKEY hKey;
    LONG result;

    // Ler da linha de comandos
    if (argc >= 2) {
        _tcscpy_s(outPipeName, size, argv[1]);

        result = RegCreateKeyEx(
            HKEY_CURRENT_USER,
            REGISTRY_KEY,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            NULL,
            &hKey,
            NULL
        );
        if (result != ERROR_SUCCESS) {
            _tprintf(_T("Erro ao criar/abrir chave do Registry (erro %ld)\n"), result);
            return FALSE;
        }

        // Escrever o nome do pipe como REG_SZ
        result = RegSetValueEx(
            hKey,
            REGISTRY_NPIPE_VALUE,
            0,
            REG_SZ,
            (const BYTE*)outPipeName,
            (DWORD)((_tcslen(outPipeName) + 1) * sizeof(TCHAR))
        );

        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            _tprintf(_T("Erro ao guardar NPIPE no Registry (erro %ld)\n"), result);
            return FALSE;
        }

        return TRUE;
    }

    // Ler do Registry
    result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0,
        KEY_READ,
        &hKey
    );
    if (result != ERROR_SUCCESS) {
        _tprintf(_T("Erro: nome do named pipe nao fornecido e chave do Registry nao encontrada.\n"));
        _tprintf(_T("Uso: placar.exe <nome_do_pipe>\n"));
        return FALSE;
    }

    DWORD cbData = size * sizeof(TCHAR); // Tamamnho maximo do buffer
    DWORD dwType;

    result = RegQueryValueEx(
        hKey,
        REGISTRY_NPIPE_VALUE,
        NULL,
        &dwType,
        (LPBYTE)outPipeName,
        &cbData
    );

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS || dwType != REG_SZ) {
        _tprintf(_T("Erro: valor NPIPE nao encontrado no Registry.\n"));
        _tprintf(_T("Uso: placar.exe <nome_do_pipe>\n"));
        return FALSE;
    }

    return TRUE;
}

BOOL ReadAlertFromRegistry(const TCHAR* pipeName, MSG_ALERTA* outAlert) {
    HKEY hKey;
    LONG result;

    // Abrir para leitura
    result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0,
        KEY_READ,
        &hKey
    );
    if (result != ERROR_SUCCESS) {
        _tprintf(_T("Erro ao abrir Registry para ler alerta (erro %ld)\n"), result);
        return FALSE;
    }

    DWORD cbData = sizeof(MSG_ALERTA);
    DWORD dwType;

    // Ler nome do pipe
    result = RegQueryValueEx(
        hKey,
        pipeName,
        NULL,
        &dwType,
        (LPBYTE)outAlert,
        &cbData
    );

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        _tprintf(_T("Erro: alerta nao encontrado no Registry (erro %ld)\n"), result);
        return FALSE;
    }

    if (dwType != REG_BINARY) {
        _tprintf(_T("Erro: valor '%s' no Registry nao e do tipo BINARY.\n"), pipeName);
        return FALSE;
    }

    if (cbData != sizeof(MSG_ALERTA)) {
        _tprintf(_T("Erro: tamanho do alerta no Registry (%lu bytes) nao corresponde ao esperado (%zu bytes).\n"),
            cbData, sizeof(MSG_ALERTA));
        return FALSE;
    }

    return TRUE;
}
