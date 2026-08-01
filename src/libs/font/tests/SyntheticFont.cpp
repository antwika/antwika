#include "SyntheticFont.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace antwika::font::tests
{

    namespace
    {
        using Bytes = std::vector<std::uint8_t>;

        constexpr int kUnitsPerEm = 1000;
        constexpr int kAscent = 800;
        constexpr int kDescent = -200;
        constexpr int kLineGap = 100;
        constexpr std::uint16_t kGlyphCount = 4;
        constexpr std::size_t kOffsetTableSize = 12;
        constexpr std::size_t kTableRecordSize = 16;

        void writeU8(Bytes &into, std::uint8_t value)
        {
            into.push_back(value);
        }

        void writeU16(Bytes &into, std::uint32_t value)
        {
            writeU8(into, static_cast<std::uint8_t>(value >> 8));
            writeU8(into, static_cast<std::uint8_t>(value));
        }

        void writeI16(Bytes &into, int value)
        {
            writeU16(into, static_cast<std::uint32_t>(value) & 0xFFFFu);
        }

        void writeU32(Bytes &into, std::uint32_t value)
        {
            writeU16(into, value >> 16);
            writeU16(into, value & 0xFFFFu);
        }

        void writeTag(Bytes &into, std::string_view tag)
        {
            for (const char letter : tag)
            {
                writeU8(into, static_cast<std::uint8_t>(letter));
            }
        }

        void padToFour(Bytes &into)
        {
            while (into.size() % 4 != 0)
            {
                writeU8(into, 0);
            }
        }

        [[nodiscard]] Bytes headTable()
        {
            Bytes table;

            writeU32(table, 0x00010000);
            writeU32(table, 0x00010000);
            writeU32(table, 0);
            writeU32(table, 0x5F0F3CF5);
            writeU16(table, 0);
            writeU16(table, kUnitsPerEm);
            writeU32(table, 0);
            writeU32(table, 0);
            writeU32(table, 0);
            writeU32(table, 0);
            writeI16(table, 0);
            writeI16(table, 0);
            writeI16(table, kUnitsPerEm);
            writeI16(table, kUnitsPerEm);
            writeU16(table, 0);
            writeU16(table, 8);
            writeI16(table, 2);

            // Long offsets in loca, so nothing is halved on the way in.
            writeI16(table, 1);
            writeI16(table, 0);

            return table;
        }

        [[nodiscard]] Bytes hheaTable()
        {
            Bytes table;

            writeU32(table, 0x00010000);
            writeI16(table, kAscent);
            writeI16(table, kDescent);
            writeI16(table, kLineGap);
            writeU16(table, 900);
            writeI16(table, 0);
            writeI16(table, 0);
            writeI16(table, 900);
            writeI16(table, 1);
            writeI16(table, 0);
            writeI16(table, 0);

            for (int reserved = 0; reserved < 4; ++reserved)
            {
                writeI16(table, 0);
            }

            writeI16(table, 0);
            writeU16(table, kGlyphCount);

            return table;
        }

        [[nodiscard]] Bytes maxpTable()
        {
            Bytes table;

            writeU32(table, 0x00010000);
            writeU16(table, kGlyphCount);

            for (int rest = 0; rest < 13; ++rest)
            {
                writeU16(table, 0);
            }

            return table;
        }

        [[nodiscard]] Bytes hmtxTable()
        {
            Bytes table;

            writeU16(table, 500);
            writeI16(table, 0);
            writeU16(table, 400);
            writeI16(table, 0);
            writeU16(table, 900);
            writeI16(table, 110);
            writeU16(table, 500);
            writeI16(table, 10);

            return table;
        }

        // One filled rectangle, four on-curve points, one contour.
        // The corners sit off the pixel grid at the sizes tested.
        // So a box is the same whole number of pixels either way.
        [[nodiscard]] Bytes rectangleGlyph(
            int xMin, int yMin, int xMax, int yMax)
        {
            Bytes glyph;

            writeI16(glyph, 1);
            writeI16(glyph, xMin);
            writeI16(glyph, yMin);
            writeI16(glyph, xMax);
            writeI16(glyph, yMax);
            writeU16(glyph, 3);
            writeU16(glyph, 0);

            for (int point = 0; point < 4; ++point)
            {
                writeU8(glyph, 0x01);
            }

            writeI16(glyph, xMin);
            writeI16(glyph, xMax - xMin);
            writeI16(glyph, 0);
            writeI16(glyph, xMin - xMax);

            writeI16(glyph, yMin);
            writeI16(glyph, 0);
            writeI16(glyph, yMax - yMin);
            writeI16(glyph, 0);

            padToFour(glyph);

            return glyph;
        }

        [[nodiscard]] Bytes glyfTable()
        {
            Bytes table = rectangleGlyph(110, 10, 890, 710);
            const Bytes second = rectangleGlyph(10, 10, 410, 410);

            table.insert(table.end(), second.begin(), second.end());

            return table;
        }

        [[nodiscard]] Bytes locaTable()
        {
            const auto first = static_cast<std::uint32_t>(
                rectangleGlyph(110, 10, 890, 710).size());
            const auto second = static_cast<std::uint32_t>(
                rectangleGlyph(10, 10, 410, 410).size());

            Bytes table;

            writeU32(table, 0);
            writeU32(table, 0);
            writeU32(table, 0);
            writeU32(table, first);
            writeU32(table, first + second);

            return table;
        }

        // A format 6 subtable: a first code and a run of glyph ids.
        // It is the simplest cmap stb_truetype understands.
        [[nodiscard]] Bytes cmapTable()
        {
            constexpr std::uint16_t kFirstCode = 0x20;
            constexpr std::uint16_t kEntryCount = 35;

            Bytes table;

            writeU16(table, 0);
            writeU16(table, 1);
            writeU16(table, 3);
            writeU16(table, 1);
            writeU32(table, 12);

            writeU16(table, 6);
            writeU16(table, 10 + 2 * kEntryCount);
            writeU16(table, 0);
            writeU16(table, kFirstCode);
            writeU16(table, kEntryCount);

            for (std::uint16_t entry = 0; entry < kEntryCount; ++entry)
            {
                const auto codepoint
                    = static_cast<char32_t>(kFirstCode + entry);

                std::uint16_t glyph = 0;

                if (codepoint == U' ')
                {
                    glyph = 1;
                }
                else if (codepoint == U'A')
                {
                    glyph = 2;
                }
                else if (codepoint == U'B')
                {
                    glyph = 3;
                }

                writeU16(table, glyph);
            }

            return table;
        }

        // Format 0, horizontal, one pair.
        // That is the only shape stb_truetype reads 'kern' in.
        [[nodiscard]] Bytes kernTable()
        {
            Bytes table;

            writeU16(table, 0);
            writeU16(table, 1);
            writeU16(table, 0);
            writeU16(table, 20);
            writeU16(table, 1);
            writeU16(table, 1);
            writeU16(table, 6);
            writeU16(table, 0);
            writeU16(table, 0);
            writeU16(table, 2);
            writeU16(table, 3);
            writeI16(table, -100);

            return table;
        }

        struct Table
        {
            std::string tag;
            Bytes bytes;
        };
    } // namespace

    std::vector<std::uint8_t> buildFont(FontRecipe recipe)
    {
        std::vector<Table> tables;

        tables.push_back({"cmap", cmapTable()});
        tables.push_back({"glyf", glyfTable()});
        tables.push_back({"head", headTable()});
        tables.push_back({"hhea", hheaTable()});
        tables.push_back({"hmtx", hmtxTable()});

        if (recipe.kerning)
        {
            tables.push_back({"kern", kernTable()});
        }

        tables.push_back({"loca", locaTable()});
        tables.push_back({"maxp", maxpTable()});

        Bytes font;

        writeU32(font, recipe.flavour);
        writeU16(font, static_cast<std::uint32_t>(tables.size()));
        writeU16(font, 0);
        writeU16(font, 0);
        writeU16(font, 0);

        auto offset = static_cast<std::uint32_t>(
            kOffsetTableSize + kTableRecordSize * tables.size());

        for (const Table &table : tables)
        {
            writeTag(font, table.tag);
            writeU32(font, 0);
            writeU32(font, offset);
            writeU32(font, static_cast<std::uint32_t>(
                table.bytes.size()));

            offset += static_cast<std::uint32_t>(table.bytes.size());
        }

        for (const Table &table : tables)
        {
            font.insert(
                font.end(), table.bytes.begin(), table.bytes.end());
        }

        return font;
    }

    std::vector<std::uint8_t> buildDirectory(
        std::uint32_t flavour,
        std::uint16_t declaredTables,
        std::span<const TableRecord> records,
        std::size_t totalSize)
    {
        Bytes blob;

        writeU32(blob, flavour);
        writeU16(blob, declaredTables);
        writeU16(blob, 0);
        writeU16(blob, 0);
        writeU16(blob, 0);

        for (const TableRecord &record : records)
        {
            writeTag(blob, record.tag);
            writeU32(blob, 0);
            writeU32(blob, record.offset);
            writeU32(blob, record.length);
        }

        blob.resize(totalSize, 0);

        return blob;
    }

} // namespace antwika::font::tests
