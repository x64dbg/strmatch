#include "plugin.h"
#include <string>
#include <unordered_map>
#include "stringutils.h"

struct StringEntry
{
    std::string utf8;
    std::string utf8_lower;
    std::wstring utf16;
    std::wstring utf16_lower;
    std::string local;
    std::string local_lower;
    std::vector<unsigned char> temp;

    StringEntry() { }

    StringEntry(const char* str)
    {
        utf16_lower = utf16 = Utf8ToUtf16(str);
        _wcslwr_s((wchar_t*)utf16_lower.c_str(), utf16_lower.size() + 1);
        local_lower = local = Utf16ToLocal(utf16.c_str());
        _strlwr_s((char*)local_lower.c_str(), local_lower.size() + 1);
        utf8 = Utf16ToUtf8(utf16.c_str());
        utf8_lower = Utf16ToUtf8(utf16_lower.c_str());
        temp.resize(max(max(utf8.length(), utf16.length()), local.length()));
    }

    bool HasValidLocal()
    {
        if(utf8.empty())
            return false;
        return !local.empty();
    }

    enum MatchMode
    {
        strcmp_utf8,
        strcmp_utf16,
        strcmp_local,
        strstr_utf8,
        strstr_utf16,
        strstr_local
    };

    static void Utf8ToLower(void* data, size_t len)
    {
        std::string str((const char*)data, len);
        auto wstr = Utf8ToUtf16(str);
        _wcslwr_s((wchar_t*)wstr.c_str(), wstr.size() + 1);
        str = Utf16ToUtf8(wstr);
        memcpy(data, str.c_str(), len);
    }

    static void Utf16ToLower(void* data, size_t len)
    {
        std::wstring str((const wchar_t*)data, len / sizeof(wchar_t));
        _wcslwr_s((wchar_t*)str.c_str(), str.size() + 1);
        memcpy(data, str.c_str(), len);
    }

    static void LocalToLower(void* data, size_t len)
    {
        std::string str((const char*)data, len);
        _strlwr_s((char*)str.c_str(), str.size() + 1);
        memcpy(data, str.c_str(), len);
    }

    bool IsMatch(duint va, MatchMode mode, bool caseSensitive)
    {
        typedef void(*TOLOWER)(void*, size_t);
        TOLOWER tolower = nullptr;
        const void* data = nullptr;
        size_t size = 0;
        switch(mode)
        {
        case strcmp_utf8:
            size = (caseSensitive ? utf8 : utf8_lower).length() + 1;
            data = (caseSensitive ? utf8 : utf8_lower).c_str();
            tolower = Utf8ToLower;
            break;
        case strcmp_utf16:
            size = ((caseSensitive ? utf16 : utf16_lower).length() + 1) * sizeof(wchar_t);
            data = (caseSensitive ? utf16 : utf16_lower).c_str();
            tolower = Utf16ToLower;
            break;
        case strcmp_local:
            size = (caseSensitive ? local : local_lower).length() + 1;
            data = (caseSensitive ? local : local_lower).c_str();
            tolower = LocalToLower;
            break;
        case strstr_utf8:
            size = (caseSensitive ? utf8 : utf8_lower).length();
            data = (caseSensitive ? utf8 : utf8_lower).c_str();
            tolower = Utf8ToLower;
            break;
        case strstr_utf16:
            size = (caseSensitive ? utf16 : utf16_lower).length() * sizeof(wchar_t);
            data = (caseSensitive ? utf16 : utf16_lower).c_str();
            tolower = Utf16ToLower;
            break;
        case strstr_local:
            size = (caseSensitive ? local : local_lower).length();
            data = (caseSensitive ? local : local_lower).c_str();
            tolower = LocalToLower;
            break;
        default:
            __debugbreak();
        }
        if(!DbgMemRead(va, temp.data(), size))
            return false;
        if(!caseSensitive)
            tolower(temp.data(), size);
        return memcmp(temp.data(), data, size) == 0;
    }
};

static std::unordered_map<duint, StringEntry> store;

static duint magic_match(duint va, duint index, StringEntry::MatchMode mode, bool caseSensitive)
{
    auto found = store.find(index);
    if(found == store.end())
    {
        _plugin_logprintf("[" PLUGIN_NAME "] Error: unknown index 0x%llX\n", uint64_t(index));
        return 1;
    }
    if((mode == StringEntry::strcmp_local || mode == StringEntry::strstr_local) && !found->second.HasValidLocal())
    {
        _plugin_logprintf("[" PLUGIN_NAME "] Error: index 0x%llX is not convertible to local\n", uint64_t(index));
        return 1;
    }
    return found->second.IsMatch(va, mode, caseSensitive) ? 1 : 0;
}

//Initialize your plugin data here.
bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
    _plugin_registercommand(pluginHandle, "strmatch_set", [](int argc, char* argv[])
    {
        if(argc < 3)
        {
            _plugin_logputs("[" PLUGIN_NAME "] Usage: strmatch_set index, \"string\"");
            return false;
        }
        bool success;
        duint index = DbgEval(argv[1], &success);
        if(!success)
        {
            _plugin_logprintf("[" PLUGIN_NAME "] Invalid expression \"%s\"\n", argv[1]);
            return false;
        }
        StringEntry entry(argv[2]);
        std::string conversions = "utf8, utf16";
        if(entry.HasValidLocal())
            conversions += ", local";
        store[index] = std::move(entry);
        _plugin_logprintf("[" PLUGIN_NAME "] String at index 0x%llX set (%s)\n", uint64_t(index), conversions.c_str());
        return true;
    }, false);
    _plugin_registercommand(pluginHandle, "strmatch_show", [](int argc, char* argv[])
    {
        if(argc < 2)
        {
            _plugin_logputs("[" PLUGIN_NAME "] Usage: strmatch_show index");
            return false;
        }
        bool success;
        duint index = DbgEval(argv[1], &success);
        if(!success)
        {
            _plugin_logprintf("[" PLUGIN_NAME "] Invalid expression \"%s\"\n", argv[1]);
            return false;
        }
        auto found = store.find(index);
        if(found == store.end())
        {
            _plugin_logprintf("[" PLUGIN_NAME "] Error: unknown index 0x%llX\n", uint64_t(index));
            return false;
        }
        auto & entry = found->second;
        _plugin_logprintf(
            "[" PLUGIN_NAME "] Index 0x%llX:\n"
            "  utf8:        \"%s\"\n"
            "  utf8_lower:  \"%s\"\n"
            "  utf16:       \"%s\"\n"
            "  utf16_lower: \"%s\"\n"
            "  local:       \"%s\"\n"
            "  local_lower: \"%s\"\n",
            uint64_t(index),
            entry.utf8.c_str(),
            entry.utf8_lower.c_str(),
            Utf16ToUtf8(entry.utf16).c_str(),
            Utf16ToUtf8(entry.utf16_lower).c_str(),
            LocalCpToUtf8(entry.local).c_str(),
            LocalCpToUtf8(entry.local_lower).c_str()
        );
        return true;
    }, false);

    _plugin_registerexprfunction(pluginHandle, "strcmp_utf8", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strcmp_utf8, true);
    }, nullptr);
    _plugin_registerexprfunction(pluginHandle, "strcmp_utf16", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strcmp_utf16, true);
    }, nullptr);
    _plugin_registerexprfunction(pluginHandle, "strcmp_local", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strcmp_local, true);
    }, nullptr);

    _plugin_registerexprfunction(pluginHandle, "stricmp_utf8", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strcmp_utf8, false);
    }, nullptr);
    _plugin_registerexprfunction(pluginHandle, "stricmp_utf16", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strcmp_utf16, false);
    }, nullptr);
    _plugin_registerexprfunction(pluginHandle, "stricmp_local", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strcmp_local, false);
    }, nullptr);

    _plugin_registerexprfunction(pluginHandle, "strstr_utf8", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strstr_utf8, true);
    }, nullptr);
    _plugin_registerexprfunction(pluginHandle, "strstr_utf16", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strstr_utf16, true);
    }, nullptr);
    _plugin_registerexprfunction(pluginHandle, "strstr_local", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strstr_local, true);
    }, nullptr);

    _plugin_registerexprfunction(pluginHandle, "stristr_utf8", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strstr_utf8, false);
    }, nullptr);
    _plugin_registerexprfunction(pluginHandle, "stristr_utf16", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strstr_utf16, false);
    }, nullptr);
    _plugin_registerexprfunction(pluginHandle, "stristr_local", 2, [](int, duint* argv, void*)
    {
        return magic_match(argv[0], argv[1], StringEntry::strstr_local, false);
    }, nullptr);

    _plugin_logputs(
        "[" PLUGIN_NAME "] documentation:\n"
        "  1. Command: strmatch_set index, \"string\"\n"
        "  2. Expression function: [action]_[encoding](va, index)\n"
        "  Actions: strcmp, stricmp, strstr, stristr\n"
        "  Encodings: utf8, utf16, local\n"
        "  Example: strcmp_utf16(va, index)"
    );
    return true;
}

//Deinitialize your plugin data here (clearing menus optional).
bool pluginStop()
{
    return true;
}

//Do GUI/Menu related things here.
void pluginSetup()
{
}
