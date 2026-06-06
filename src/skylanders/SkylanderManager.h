#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include "SkylanderInfo.h"

namespace ssa::Skylanders
{
    // represents a loaded figure in the user's library
    struct LoadedSkylander
    {
        std::filesystem::path filePath;
        std::string filename;
        std::string displayName;
        std::string nickName;
        uint16_t figureId;
        uint16_t variantId;
    };

    class SkylanderManager
    {
    public:
        static SkylanderManager& Get()
        {
            static SkylanderManager instance;
            return instance;
        }

        void Initialize();
        void RefreshLibrary();
        const std::vector<LoadedSkylander>& GetLibrary() const { return m_library; }

        static void GenerateFigureData(uint16_t figureId, uint16_t variantId, uint8_t* outBuffer);
        bool CreateNewFigure(uint16_t figureId, uint16_t variantId, const std::string& nickname);

        static bool ReadFigureData(const std::filesystem::path& path, uint8_t* outBuffer);
        static bool SaveFigureData(const std::filesystem::path& path, const uint8_t* inBuffer);

        // Database lookups
        static const SkylanderInfo* FindFigure(uint16_t figureId, uint16_t variantId);
        static const SkylanderCategory* FindCategoryByFigureId(uint16_t figureId);

        static std::string GetVariantLabel(const SkylanderInfo& fig);

    private:
        SkylanderManager() = default;
        ~SkylanderManager() = default;

        std::vector<LoadedSkylander> m_library;
        std::filesystem::path m_baseDirectory;

        // core data logic
        static uint16_t CalculateCRC16(uint16_t initValue, const uint8_t* buffer, uint32_t size);
        static std::string SanitizeForFilename(const std::string& input);
    };
} // namespace ssa::Skylanders
