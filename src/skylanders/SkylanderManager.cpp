// Portions of this file are derived from Cemu (https://github.com/cemu-project/Cemu)
// Licensed under Mozilla Public License 2.0 - see THIRD_PARTY_NOTICES.md
#include "SkylanderManager.h"
#include <fstream>
#include <algorithm>
#include <array>
#include <random>
#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <windows.h>

#include "log.h"
#include "imgui/fonts/IconsSkylanders.h"

namespace ssa::Skylanders
{
    // -------------------------------------------------------------------------
    // Setup & Library Management
    // -------------------------------------------------------------------------

    void SkylanderManager::Initialize()
    {
        char modulePath[MAX_PATH];
        GetModuleFileNameA(nullptr, modulePath, MAX_PATH);

        const std::filesystem::path exePath(modulePath);

        m_baseDirectory = exePath.parent_path() / "ssa-improved" / "skylanders";

        if (!std::filesystem::exists(m_baseDirectory))
        {
            std::filesystem::create_directories(m_baseDirectory);
        }

        RefreshLibrary();
    }

    void SkylanderManager::RefreshLibrary()
    {
        m_library.clear();

        if (!std::filesystem::exists(m_baseDirectory)) return;

        // recursively scan the directory for .sky / .bin / .dump / .dmp files
        for (const auto& entry : std::filesystem::recursive_directory_iterator(m_baseDirectory))
        {
            if (entry.is_regular_file())
            {
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != ".sky" && ext != ".bin" && ext != ".dump" && ext != ".dmp")
                    continue;

                uint8_t buffer[1024];
                if (ReadFigureData(entry.path(), buffer))
                {
                    LoadedSkylander sky;
                    sky.filePath = entry.path();
                    sky.filename = entry.path().filename().string();

                    // the PC is little-endian, so we can cast directly
                    sky.figureId = *reinterpret_cast<uint16_t*>(&buffer[0x10]);
                    sky.variantId = *reinterpret_cast<uint16_t*>(&buffer[0x1C]);

                    const SkylanderInfo* info = FindFigure(sky.figureId, sky.variantId);
                    const SkylanderCategory* category = FindCategoryByFigureId(sky.figureId);

                    if (info && category) {
                        sky.displayName = GetVariantLabel(*info);

                        // reconstruct the prefix to cleanly extract the nickname
                        std::string prefix = std::string(category->cleanName);
                        if (info->fileAppend && strlen(info->fileAppend) > 0) {
                            prefix += "-";
                            prefix += info->fileAppend;
                        }
                        prefix += "-";

                        // stem() gets the filename without the extension
                        std::string rawName = sky.filePath.stem().string();

                        // extract the nickname if the file matches our naming convention
                        if (rawName.rfind(prefix, 0) == 0) {
                            sky.nickName = rawName.substr(prefix.length());
                        } else {
                            // fallback if the user manually renamed the file weirdly
                            sky.nickName = "";
                        }
                    } else {
                        sky.displayName = "Unknown Figure (" + std::to_string(sky.figureId) + ")";
                        sky.nickName = "";
                    }

                    m_library.push_back(sky);
                }
            }
        }

        Log("Skylanders Library refreshed: %d figure(s) found", m_library.size());
    }

    // -------------------------------------------------------------------------
    // Creation & File I/O
    // -------------------------------------------------------------------------

    void SkylanderManager::GenerateFigureData(uint16_t figureId, uint16_t variantId, uint8_t* outBuffer)
    {
        memset(outBuffer, 0, 1024);

        // sector trailers (Access Control Bits)
        uint32_t first_block = 0x690F0F0F;
        uint32_t other_blocks = 0x69080F7F;
        memcpy(&outBuffer[0x36], &first_block, sizeof(first_block));
        for (size_t index = 1; index < 0x10; index++) {
            memcpy(&outBuffer[(index * 0x40) + 0x36], &other_blocks, sizeof(other_blocks));
        }

        // random UID and BCC
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<int> dist(0, 255);
        outBuffer[0] = dist(mt);
        outBuffer[1] = dist(mt);
        outBuffer[2] = dist(mt);
        outBuffer[3] = dist(mt);
        outBuffer[4] = outBuffer[0] ^ outBuffer[1] ^ outBuffer[2] ^ outBuffer[3]; // BCC checksum
        outBuffer[5] = 0x81;
        outBuffer[6] = 0x01;
        outBuffer[7] = 0x0F;

        // figure and variant IDs
        memcpy(&outBuffer[0x10], &figureId, sizeof(figureId));
        memcpy(&outBuffer[0x1C], &variantId, sizeof(variantId));

        // CRC16 over first 0x1E bytes
        uint16_t crc = CalculateCRC16(0xFFFF, outBuffer, 0x1E);
        memcpy(&outBuffer[0x1E], &crc, sizeof(crc));
    }

    bool SkylanderManager::CreateNewFigure(uint16_t figureId, uint16_t variantId, const std::string& nickname)
    {
        const SkylanderInfo* info = FindFigure(figureId, variantId);
        const SkylanderCategory* category = FindCategoryByFigureId(figureId);

        if (!info || !category) return false;

        // build the directory path: ./skylanders/lightning_rod/
        std::filesystem::path targetDir = m_baseDirectory / category->cleanName;

        if (!std::filesystem::exists(targetDir)) {
            std::filesystem::create_directories(targetDir);
        }

        // build the file name: lightning_rod-s2-My_Nickname.sky
        std::string safeNickname = SanitizeForFilename(nickname);
        std::string fileName = std::string(category->cleanName);
        if (info->fileAppend && strlen(info->fileAppend) > 0) {
            fileName += std::string("-") + info->fileAppend;
        }
        fileName += "-" + safeNickname + ".sky";

        std::filesystem::path targetFile = targetDir / fileName;

        // prevent overwriting
        if (std::filesystem::exists(targetFile)) {
            return false;
        }

        // generate figure data
        std::array<uint8_t, 1024> data{};
        GenerateFigureData(figureId, variantId, data.data());

        // save and refresh
        bool saved = SaveFigureData(targetFile, data.data());
        if (saved) {
            Log("Created new figure: %s (ID: %04X, Variant: %04X) at %s", info->displayName, figureId, variantId, targetFile.string().c_str());
            RefreshLibrary();
        }
        return saved;
    }

    bool SkylanderManager::ReadFigureData(const std::filesystem::path& path, uint8_t* outBuffer)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        std::streamsize size = file.tellg();
        if (size != 1024) return false; // Ensure it's a valid dump size

        file.seekg(0, std::ios::beg);
        if (file.read(reinterpret_cast<char*>(outBuffer), size)) return true;

        return false;
    }

    bool SkylanderManager::SaveFigureData(const std::filesystem::path& path, const uint8_t* inBuffer)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) return false;

        file.write(reinterpret_cast<const char*>(inBuffer), 1024);

        return file.good();
    }

    // -------------------------------------------------------------------------
    // Utilities & Lookups
    // -------------------------------------------------------------------------

    std::string SkylanderManager::SanitizeForFilename(const std::string& input)
    {
        std::string safe = input;
        std::replace(safe.begin(), safe.end(), ' ', '_');
        safe.erase(std::remove_if(safe.begin(), safe.end(), [](char c) {
            return !std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-';
        }), safe.end());

        if (safe.empty()) {
            safe = "Unnamed";
        }
        return safe;
    }

    const SkylanderInfo* SkylanderManager::FindFigure(uint16_t figureId, uint16_t variantId)
    {
        for (const auto& category : g_skylanderDb)
        {
            for (size_t i = 0; i < category.numFigures; ++i)
            {
                if (category.figures[i].figureId == figureId && category.figures[i].variantId == variantId)
                {
                    return &category.figures[i];
                }
            }
        }

        for (const auto& figure : g_skylanderMinisDb)
        {
            if (figure.figureId == figureId)
            {
                return &figure;
            }
        }

        for (const auto& figure : g_skylanderItemsDb)
        {
            if (figure.figureId == figureId)
            {
                return &figure;
            }
        }

        for (const auto& figure : g_skylanderAdventureDb)
        {
            if (figure.figureId == figureId)
            {
                return &figure;
            }
        }

        return nullptr;
    }

    const SkylanderCategory* SkylanderManager::FindCategoryByFigureId(uint16_t figureId)
    {
        for (const auto& category : g_skylanderDb)
        {
            for (size_t i = 0; i < category.numFigures; ++i)
            {
                if (category.figures[i].figureId == figureId)
                {
                    return &category;
                }
            }
        }
        return nullptr;
    }

    uint16_t SkylanderManager::CalculateCRC16(uint16_t initValue, const uint8_t* buffer, uint32_t size)
    {
        static const uint16_t CRC_CCITT_TABLE[256] = {
            0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210, 0x3273,
            0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528,
            0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 0x48C4, 0x58E5, 0x6886,
            0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF,
            0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6, 0x5ED5,
            0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2,
            0x20E3, 0x5004, 0x4025, 0x7046, 0x6067, 0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB, 0x95A8,
            0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691,
            0x16B0, 0x6657, 0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 0xCB7D, 0xDB5C, 0xEB3F,
            0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64,
            0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
        };

        uint16_t crc = initValue;
        for (uint32_t i = 0; i < size; i++)
        {
            const uint16_t tmp = (crc >> 8) ^ buffer[i];
            crc = (crc << 8) ^ CRC_CCITT_TABLE[tmp];
        }
        return crc;
    }

    // -------------------------------------------------------------------------
    // Frontend Helper
    // -------------------------------------------------------------------------

    std::string SkylanderManager::GetVariantLabel(const SkylanderInfo& fig)
    {
        if (fig.fileAppend)
        {

            if (std::strncmp(fig.fileAppend, "s2", 2) == 0)
                return std::string(ICON_SKY_SERIES_2) + " " + fig.displayName;
            if (std::strncmp(fig.fileAppend, "s3", 2) == 0)
                return std::string(ICON_SKY_SERIES_3) + " " + fig.displayName;
            if (std::strncmp(fig.fileAppend, "s4", 2) == 0)
                return std::string(ICON_SKY_SERIES_4) + " " + fig.displayName;
            if (std::strcmp(fig.fileAppend, "lightcore") == 0)
                return std::string(ICON_SKY_LIGHTCORE) + " " + fig.displayName;
            if (std::strcmp(fig.fileAppend, "ee") == 0)
                return std::string(ICON_SKY_EONS_ELITE) + " " + fig.displayName;
            if (std::strcmp(fig.fileAppend, "mini") == 0)
                return std::string(ICON_SKY_MINIS) + " " + fig.displayName;
            if (std::strcmp(fig.fileAppend, "item") == 0)
                return std::string(ICON_SKY_VEHICLES) + " " + fig.displayName;
            if (std::strcmp(fig.fileAppend, "adventure") == 0)
                return std::string(ICON_SKY_NEW) + " " + fig.displayName;
        }

        return fig.displayName;
    }
} // namespace ssa::Skylanders