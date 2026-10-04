#pragma once
// Diagnostics tables
// - filled once per frame, drawn with ImGui and can be copied to the clipboard as tab-separated text
//
// DebugTable table{ "Name", "HP" };
// table.AddRow().Add("Zap").Add(Fmt("%.1f", hp));
// table.Show("##my_table", ImGuiTableFlags_ScrollY);   // small "Copy table" button + the table

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>
#include "imgui/ui.h"

namespace ssa::UIPages
{
    namespace TableColor
    {
        inline constexpr ImU32 Green = IM_COL32(102, 255, 153, 255);
        inline constexpr ImU32 Red = IM_COL32(255, 102, 102, 255);
        inline constexpr ImU32 Yellow = IM_COL32(204, 204, 102, 255);
        inline constexpr ImU32 Amber = IM_COL32(255, 204, 0, 255);
        inline constexpr ImU32 Purple = IM_COL32(204, 102, 255, 255);
        inline constexpr ImU32 RowHighlight = IM_COL32(51, 140, 51, 89); // e.g. the current level
    }

    // printf into a std::string (up to 255 characters)
    inline std::string Fmt(const char* format, ...)
    {
        char buf[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        return buf;
    }

    // "0x%08X" of the low 32 bits (the game is 32 bit, so this is the whole address)
    inline std::string Hex(uintptr_t value) { return Fmt("0x%08X", static_cast<uint32_t>(value)); }

    struct DebugTable
    {
        struct Cell
        {
            std::string text; // what the table shows
            std::string full; // what is copied and shown on hover, if it differs from `text`
            ImU32 color = 0; // 0 = default text colour
            bool dim = false; // greyed out
        };

        struct Row
        {
            std::vector<Cell> cells;
            ImU32 background = 0; // 0 = default

            Row& Add(std::string text, ImU32 color = 0)
            {
                cells.push_back({std::move(text), {}, color, false});
                return *this;
            }

            Row& Dim(std::string text)
            {
                cells.push_back({std::move(text), {}, 0, true});
                return *this;
            }

            Row& AddFull(std::string text, std::string full)
            {
                cells.push_back({std::move(text), std::move(full), 0, false});
                return *this;
            }
        };

        // a column that only exists on screen (buttons, checkboxes); it is not part of the copy
        struct ExtraColumn
        {
            const char* header;
            std::function<void(size_t)> draw; // called with the row index
        };

        std::vector<std::string> columns;
        std::vector<Row> rows;

        DebugTable(std::initializer_list<const char*> names)
        {
            for (const char* name : names) columns.emplace_back(name);
        }

        Row& AddRow() { return rows.emplace_back(); }

        std::string ToTSV() const
        {
            std::string out;
            for (size_t c = 0; c < columns.size(); ++c)
            {
                if (c) out += '\t';
                out += columns[c];
            }
            out += '\n';

            for (const Row& row : rows)
            {
                for (size_t c = 0; c < columns.size(); ++c)
                {
                    if (c) out += '\t';
                    if (c < row.cells.size())
                        out += row.cells[c].full.empty() ? row.cells[c].text : row.cells[c].full;
                }
                out += '\n';
            }
            return out;
        }

        void CopyButton(const char* label = "Copy table to clipboard") const
        {
            if (ImGui::Button(label))
                ImGui::SetClipboardText(ToTSV().c_str());
        }

        void Draw(const char* id, ImGuiTableFlags flags = 0, const ExtraColumn* extra = nullptr) const
        {
            const int columnCount = static_cast<int>(columns.size()) + (extra ? 1 : 0);
            if (!ImGui::BeginTable(id, columnCount,
                                   ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit |
                                   flags))
                return;

            if (flags & ImGuiTableFlags_ScrollY)
                ImGui::TableSetupScrollFreeze(0, 1);
            if (extra)
                ImGui::TableSetupColumn(extra->header);
            for (const std::string& name : columns)
                ImGui::TableSetupColumn(name.c_str());
            ImGui::TableHeadersRow();

            for (size_t r = 0; r < rows.size(); ++r)
            {
                const Row& row = rows[r];
                ImGui::TableNextRow();
                if (row.background)
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row.background);

                int column = 0;
                if (extra)
                {
                    ImGui::TableSetColumnIndex(column++);
                    extra->draw(r);
                }
                for (size_t c = 0; c < row.cells.size() && c < columns.size(); ++c)
                {
                    ImGui::TableSetColumnIndex(column++);
                    DrawCell(row.cells[c]);
                }
            }
            ImGui::EndTable();
        }

        // a small copy button above the table, then the table
        void Show(const char* id, ImGuiTableFlags flags = 0) const
        {
            ImGui::PushID(id);
            if (ImGui::SmallButton("Copy table"))
                ImGui::SetClipboardText(ToTSV().c_str());
            ImGui::PopID();
            Draw(id, flags);
        }

    private:
        static void DrawCell(const Cell& cell)
        {
            if (cell.dim)
                ImGui::TextDisabled("%s", cell.text.c_str());
            else if (cell.color)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, cell.color);
                ImGui::TextUnformatted(cell.text.c_str());
                ImGui::PopStyleColor();
            }
            else
                ImGui::TextUnformatted(cell.text.c_str());

            if (!cell.full.empty())
                UI::HoverTooltip(cell.full.c_str());
        }
    };
} // namespace ssa::UIPages
