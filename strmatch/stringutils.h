#pragma once

#include <string>

std::string Utf16ToUtf8(const wchar_t* wstr)
{
    std::string convertedString;
    if(!wstr || !*wstr)
        return convertedString;
    auto requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if(requiredSize > 0)
    {
        convertedString.resize(requiredSize - 1);
        if(!WideCharToMultiByte(CP_UTF8, 0, wstr, -1, (char*)convertedString.c_str(), requiredSize, nullptr, nullptr))
            convertedString.clear();
    }
    return convertedString;
}

std::string Utf16ToLocal(const wchar_t* wstr)
{
    std::string convertedString;
    if(!wstr || !*wstr)
        return convertedString;
    auto requiredSize = WideCharToMultiByte(CP_ACP, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if(requiredSize > 0)
    {
        convertedString.resize(requiredSize - 1);
        if(!WideCharToMultiByte(CP_ACP, 0, wstr, -1, (char*)convertedString.c_str(), requiredSize, nullptr, nullptr))
            convertedString.clear();
    }
    return convertedString;
}

std::wstring Utf8ToUtf16(const char* str)
{
    std::wstring convertedString;
    if(!str || !*str)
        return convertedString;
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    if(requiredSize > 0)
    {
        convertedString.resize(requiredSize - 1);
        if(!MultiByteToWideChar(CP_UTF8, 0, str, -1, (wchar_t*)convertedString.c_str(), requiredSize))
            convertedString.clear();
    }
    return convertedString;
}

std::wstring LocalCpToUtf16(const char* str)
{
    std::wstring convertedString;
    if(!str || !*str)
        return convertedString;
    int requiredSize = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    if(requiredSize > 0)
    {
        convertedString.resize(requiredSize - 1);
        if(!MultiByteToWideChar(CP_ACP, 0, str, -1, (wchar_t*)convertedString.c_str(), requiredSize))
            convertedString.clear();
    }
    return convertedString;
}

std::string LocalCpToUtf8(const char* str)
{
    return Utf16ToUtf8(LocalCpToUtf16(str).c_str());
}

std::wstring Utf8ToUtf16(const std::string & str)
{
    return Utf8ToUtf16(str.c_str());
}

std::string Utf16ToUtf8(const std::wstring & wstr)
{
    return Utf16ToUtf8(wstr.c_str());
}

std::string LocalCpToUtf8(const std::string & str)
{
    return LocalCpToUtf8(str.c_str());
}

std::wstring LocalCpToUtf16(const std::string & str)
{
    return LocalCpToUtf16(str.c_str());
}