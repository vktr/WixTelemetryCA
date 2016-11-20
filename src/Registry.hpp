#pragma once

#include <string>
#include <windows.h>

class Registry
{
public:
    Registry(HKEY hKey, std::wstring const& subKey);

    bool GetValue(std::wstring const& value, int* out);
    bool GetValue(std::wstring const& value, std::wstring* out);

private:
    HKEY m_hKey;
    std::wstring m_subKey;
};
