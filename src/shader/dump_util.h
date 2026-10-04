#pragma once
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include "../window/texture_mods.h"

// Where the mod writes its shader dumps, and a tiny RAII wrapper around FILE* for writing them.
namespace ssa::DumpUtil
{
    // <mod dir>/dumps/shaders, created if it does not exist yet
    inline std::filesystem::path ShaderDumpDir()
    {
        const std::filesystem::path dir = std::filesystem::path(TextureMods::GetModDir()) / "dumps" / "shaders";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        return dir;
    }

    // usage: if (OutFile f{ path }) { f.Printf(...); } (the file is closed when f goes out of scope)
    class OutFile
    {
    public:
        explicit OutFile(const std::filesystem::path& path, bool binary = false)
        {
            if (_wfopen_s(&m_file, path.c_str(), binary ? L"wb" : L"w") != 0)
                m_file = nullptr;
        }

        OutFile(const OutFile&) = delete;
        OutFile& operator=(const OutFile&) = delete;
        ~OutFile() { if (m_file) fclose(m_file); }

        explicit operator bool() const { return m_file != nullptr; }

        void Write(const void* data, size_t elementSize, size_t count) { fwrite(data, elementSize, count, m_file); }
        void Write(const std::string& text) { fwrite(text.data(), 1, text.size(), m_file); }
        void Puts(const char* text) { fputs(text, m_file); }

        void Printf(const char* format, ...)
        {
            va_list args;
            va_start(args, format);
            vfprintf(m_file, format, args);
            va_end(args);
        }

    private:
        FILE* m_file = nullptr;
    };
} // namespace ssa::DumpUtil
