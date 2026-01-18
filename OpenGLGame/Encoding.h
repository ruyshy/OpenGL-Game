#pragma once

#include <string>
#include <string_view>
#include <filesystem>

namespace util::encoding
{
    std::wstring to_wide(std::string_view s, unsigned int codepage);
    std::string  to_multibyte(std::wstring_view ws, unsigned int codepage);

    inline std::string utf8_to_acp(std::string_view utf8)
    {
        return to_multibyte(to_wide(utf8, 65001 /*CP_UTF8*/), 0 /*CP_ACP*/);
    }

    inline std::string acp_to_utf8(std::string_view acp)
    {
        return to_multibyte(to_wide(acp, 0 /*CP_ACP*/), 65001 /*CP_UTF8*/);
    }
}

namespace util::fs
{
    bool write_file(const std::filesystem::path& path, std::string_view bytes);
}
