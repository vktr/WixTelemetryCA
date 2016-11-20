#include <windows.h>

#include <msi.h>
#include <msiquery.h>
#include <strsafe.h>
#include <wcautil.h>
#include <winhttp.h>

void Log(LPWSTR text)
{
    int size = WideCharToMultiByte(CP_UTF8, 0, text, wcslen(text), NULL, 0, NULL, NULL);
    CHAR* result = new CHAR[size];
    WideCharToMultiByte(CP_UTF8, 0, text, wcslen(text), result, size, NULL, NULL);

    WcaLog(LOGMSG_STANDARD, result);
}

extern "C" __declspec(dllexport) UINT __stdcall CollectTelemetry(MSIHANDLE hInstall)
{
    HRESULT hRes = WcaInitialize(hInstall, "CollectTelemetry");
    ExitOnFailure(hRes, "Failed to initialize");

    LPWSTR szTargetUrl = NULL;
    hRes = WcaGetProperty(L"TELEMETRY_TARGET_URL", &szTargetUrl);
    ExitOnFailure(hRes, "Property TELEMETRY_TARGET_URL not set");

    Log(szTargetUrl);
    WcaLog(LOGMSG_STANDARD, "Collecting (anonymous) telemetry");

    HINTERNET hSession = WinHttpOpen(
        L"WixTelemetryCA",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC);

    if (hSession == NULL)
    {
        WcaLog(LOGMSG_STANDARD, "WinHttpOpen failed");
        ExitFunctionWithLastError(hRes);
    }

    // Crack URI
    URL_COMPONENTS uc = { sizeof(URL_COMPONENTS) };
    uc.dwSchemeLength = (DWORD)-1;
    uc.dwHostNameLength = (DWORD)-1;
    uc.dwUrlPathLength = (DWORD)-1;
    uc.dwExtraInfoLength = (DWORD)-1;
    
    if (!WinHttpCrackUrl(szTargetUrl, wcslen(szTargetUrl), 0, &uc))
    {
        WcaLog(LOGMSG_STANDARD, "WinHttpCrackUrl failed");
        ExitFunctionWithLastError(hRes);
    }

    LPTSTR szHostName = new TCHAR[uc.dwHostNameLength];
    wcsncpy_s(szHostName, uc.dwHostNameLength + 1, uc.lpszHostName, _TRUNCATE);

    Log(szHostName);

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        szHostName,
        uc.nPort,
        NULL);

    delete[] szHostName;

    if (hConnect == NULL)
    {
        WcaLog(LOGMSG_STANDARD, "WinHttpConnect failed");
        ExitFunctionWithLastError(hRes);
    }

    bool secure = lstrcmp(uc.lpszScheme, TEXT("https")) == 0;

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        uc.lpszUrlPath,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    if (hRequest == NULL)
    {
        WcaLog(LOGMSG_STANDARD, "WinHttpOpenRequest failed");
        ExitFunctionWithLastError(hRes);
    }

    WinHttpAddRequestHeaders(
        hRequest,
        TEXT("Content-Type: application/json"),
        -1,
        WINHTTP_ADDREQ_FLAG_ADD);

    OSVERSIONINFO vi;
    vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&vi))
    {
        WcaLog(LOGMSG_STANDARD, "GetVersionEx failed");
        ExitFunctionWithLastError(hRes);
    }

    char version[1024];
    snprintf(
        version,
        ARRAYSIZE(version),
        "{ \"dwBuildNumber\": %d, \"dwMajorVersion\": %d, \"dwMinorVersion\": %d, \"dwPlatformId\": %d }",
        vi.dwBuildNumber, vi.dwMajorVersion, vi.dwMinorVersion, vi.dwPlatformId);

    WcaLog(LOGMSG_STANDARD, "Sending (anonymous) telemetry data");

    if (!WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        NULL,
        version,
        strlen(version),
        strlen(version),
        NULL))
    {
        char buf[256];
        snprintf(buf, ARRAYSIZE(buf), "WinHttpSendRequest failed with error: %d", GetLastError());
        WcaLog(LOGMSG_STANDARD, buf);
        ExitFunctionWithLastError(hRes);
    }

    WcaLog(LOGMSG_STANDARD, "Telemetry sent");

LExit:
    if (hRequest != NULL) { WinHttpCloseHandle(hRequest); }
    if (hConnect != NULL) { WinHttpCloseHandle(hConnect); }
    if (hSession != NULL) { WinHttpCloseHandle(hSession); }

    UINT err = SUCCEEDED(hRes)
        ? ERROR_SUCCESS
        : ERROR_INSTALL_FAILURE;

    return WcaFinalize(err);
}

extern "C" BOOL WINAPI DllMain(
    __in HINSTANCE hInst,
    __in ULONG ulReason,
    __in LPVOID)
{
    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        WcaGlobalInitialize(hInst);
        break;
    case DLL_PROCESS_DETACH:
        WcaGlobalFinalize();
        break;
    }

    return TRUE;
}
