#include <windows.h>

#include <msi.h>
#include <msiquery.h>
#include <stdlib.h>
#include <strsafe.h>
#include <wcautil.h>
#include <winhttp.h>

#include "picojson.hpp"
#include "Registry.hpp"

void Log(LPWSTR text)
{
    int size = WideCharToMultiByte(CP_UTF8, 0, text, wcslen(text), NULL, 0, NULL, NULL);
    CHAR* result = new CHAR[size];
    WideCharToMultiByte(CP_UTF8, 0, text, wcslen(text), result, size, NULL, NULL);

    WcaLog(LOGMSG_STANDARD, result);
}

static std::string ToString(const std::wstring &str)
{
    int size = WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0, NULL, NULL);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size, NULL, NULL);
    return result;
}

extern "C" __declspec(dllexport) UINT __stdcall CollectTelemetry(MSIHANDLE hInstall)
{
    HRESULT hRes = WcaInitialize(hInstall, "CollectTelemetry");

    LPWSTR szTargetUrl = NULL;
    hRes = WcaGetProperty(L"TELEMETRY_TARGET_URL", &szTargetUrl);

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
    }

    WinHttpAddRequestHeaders(
        hRequest,
        TEXT("Content-Type: application/json"),
        -1,
        WINHTTP_ADDREQ_FLAG_ADD);

    picojson::object os;

    Registry reg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");

    int currentMajorVersionNumber;
    if (reg.GetValue(L"CurrentMajorVersionNumber", &currentMajorVersionNumber))
    {
        os.insert(std::make_pair("currentMajorVersionNumber", picojson::value((double)currentMajorVersionNumber)));
    }

    std::wstring currentVersion;
    if (reg.GetValue(L"CurrentVersion", &currentVersion))
    {
        os.insert(std::make_pair("currentVersion", picojson::value(ToString(currentVersion))));
    }

    std::wstring productName;
    if (reg.GetValue(L"ProductName", &productName))
    {
        os.insert(std::make_pair("productName", picojson::value(ToString(productName))));
    }

    WcaLog(LOGMSG_STANDARD, "Sending (anonymous) telemetry data");
    std::string json = picojson::value(os).serialize();

    if (!WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        NULL,
        reinterpret_cast<LPVOID>(&json[0]),
        json.size(),
        json.size(),
        NULL))
    {
        WcaLog(LOGMSG_STANDARD, "WinHttpSendRequest failed");
    }

    WcaLog(LOGMSG_STANDARD, "Telemetry sent");

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
