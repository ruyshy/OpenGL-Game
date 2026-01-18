#include "pch.h"
#include "Encoding.h"

#include <fstream>

namespace util::encoding
{
    std::wstring to_wide(std::string_view s, unsigned int codepage)
    {
#ifdef _WIN32
        if (s.empty()) return {};

        int required = ::MultiByteToWideChar(
            codepage, 0,
            s.data(), static_cast<int>(s.size()),
            nullptr, 0);

        if (required <= 0) return {};

        std::wstring out(required, L'\0');

        int written = ::MultiByteToWideChar(
            codepage, 0,
            s.data(), static_cast<int>(s.size()),
            out.data(), required);

        if (written <= 0) return {};
        return out;
#else
        (void)s; (void)codepage;
        return {};
#endif
    }

    std::string to_multibyte(std::wstring_view ws, unsigned int codepage)
    {
#ifdef _WIN32
        if (ws.empty()) return {};

        int required = ::WideCharToMultiByte(
            codepage, 0,
            ws.data(), static_cast<int>(ws.size()),
            nullptr, 0, nullptr, nullptr);

        if (required <= 0) return {};

        std::string out(required, '\0');

        int written = ::WideCharToMultiByte(
            codepage, 0,
            ws.data(), static_cast<int>(ws.size()),
            out.data(), required, nullptr, nullptr);

        if (written <= 0) return {};
        return out;
#else
        (void)ws; (void)codepage;
        return {};
#endif
    }
}

namespace util::fs
{
    bool write_file(const std::filesystem::path& path, std::string_view bytes)
    {
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) return false;

        ofs.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return static_cast<bool>(ofs);
    }
}
