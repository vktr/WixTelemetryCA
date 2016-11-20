#include "Registry.hpp"

Registry::Registry(HKEY hKey, std::wstring const& subKey)
    : m_hKey(hKey),
    m_subKey(subKey)
{
}

bool Registry::GetValue(std::wstring const& value, int* out)
{
    DWORD Value = 0; // ValueBuffer[BUFFER];
    DWORD BufferSize = sizeof(DWORD);

    LSTATUS lStatus = RegGetValue(
        m_hKey,
        m_subKey.c_str(),
        value.c_str(),
        RRF_RT_REG_DWORD,
        NULL,
        &Value,
        &BufferSize);

    if (lStatus != ERROR_SUCCESS)
    {
        return false;
    }

    *out = static_cast<int>(Value);
    return true;
}

bool Registry::GetValue(std::wstring const& value, std::wstring* out)
{
    TCHAR Value[8192]; // ValueBuffer[BUFFER];
    DWORD BufferSize = 8192;

    LSTATUS lStatus = RegGetValue(
        m_hKey,
        m_subKey.c_str(),
        value.c_str(),
        RRF_RT_REG_SZ,
        NULL,
        &Value,
        &BufferSize);

    if (lStatus != ERROR_SUCCESS)
    {
        return false;
    }

    *out = Value;
    return true;
}
