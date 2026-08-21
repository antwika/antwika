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

        void writeU8(Bytes &intoBytes, std::uint8_t value)
        {
            intoBytes.push_back(value);
        }

        void writeU16(Bytes &intoBytes, std::uint32_t value)
        {
            writeU8(intoBytes, static_cast<std::uint8_t>(value >> 8));
            writeU8(intoBytes, static_cast<std::uint8_t>(value));
        }

        void writeI16(Bytes &intoBytes, int value)
        {
            writeU16(intoBytes, static_cast<std::uint32_t>(value) & 0xFFFFu);
        }

        void writeU32(Bytes &intoBytes, std::uint32_t value)
        {
            writeU16(intoBytes, value >> 16);
            writeU16(intoBytes, value & 0xFFFFu);
        }

        void writeTag(Bytes &intoBytes, std::string_view tag)
        {
            for (const char letter : tag)
            {
                writeU8(intoBytes, static_cast<std::uint8_t>(letter));
            }
        }

        void padToFour(Bytes &intoBytes)
        {
            while (intoBytes.size() % 4 != 0)
            {
                writeU8(intoBytes, 0);
            }
        }

        [[nodiscard]] Bytes headTable()
        {
            Bytes tableBytes;

            writeU32(tableBytes, 0x00010000);
            writeU32(tableBytes, 0x00010000);
            writeU32(tableBytes, 0);
            writeU32(tableBytes, 0x5F0F3CF5);
            writeU16(tableBytes, 0);
            writeU16(tableBytes, kUnitsPerEm);
            writeU32(tableBytes, 0);
            writeU32(tableBytes, 0);
            writeU32(tableBytes, 0);
            writeU32(tableBytes, 0);
            writeI16(tableBytes, 0);
            writeI16(tableBytes, 0);
            writeI16(tableBytes, kUnitsPerEm);
            writeI16(tableBytes, kUnitsPerEm);
            writeU16(tableBytes, 0);
            writeU16(tableBytes, 8);
            writeI16(tableBytes, 2);

            writeI16(tableBytes, 1);
            writeI16(tableBytes, 0);

            return tableBytes;
        }

        [[nodiscard]] Bytes hheaTable()
        {
            Bytes tableBytes;

            writeU32(tableBytes, 0x00010000);
            writeI16(tableBytes, kAscent);
            writeI16(tableBytes, kDescent);
            writeI16(tableBytes, kLineGap);
            writeU16(tableBytes, 900);
            writeI16(tableBytes, 0);
            writeI16(tableBytes, 0);
            writeI16(tableBytes, 900);
            writeI16(tableBytes, 1);
            writeI16(tableBytes, 0);
            writeI16(tableBytes, 0);

            for (int index = 0; index < 4; ++index)
            {
                writeI16(tableBytes, 0);
            }

            writeI16(tableBytes, 0);
            writeU16(tableBytes, kGlyphCount);

            return tableBytes;
        }

        [[nodiscard]] Bytes maxpTable()
        {
            Bytes tableBytes;

            writeU32(tableBytes, 0x00010000);
            writeU16(tableBytes, kGlyphCount);

            for (int rest = 0; rest < 13; ++rest)
            {
                writeU16(tableBytes, 0);
            }

            return tableBytes;
        }

        [[nodiscard]] Bytes hmtxTable()
        {
            Bytes tableBytes;

            writeU16(tableBytes, 500);
            writeI16(tableBytes, 0);
            writeU16(tableBytes, 400);
            writeI16(tableBytes, 0);
            writeU16(tableBytes, 900);
            writeI16(tableBytes, 110);
            writeU16(tableBytes, 500);
            writeI16(tableBytes, 10);

            return tableBytes;
        }

        [[nodiscard]] Bytes rectangleGlyph(
            int xMin, int yMin, int xMax, int yMax)
        {
            Bytes glyphBytes;

            writeI16(glyphBytes, 1);
            writeI16(glyphBytes, xMin);
            writeI16(glyphBytes, yMin);
            writeI16(glyphBytes, xMax);
            writeI16(glyphBytes, yMax);
            writeU16(glyphBytes, 3);
            writeU16(glyphBytes, 0);

            for (int point = 0; point < 4; ++point)
            {
                writeU8(glyphBytes, 0x01);
            }

            writeI16(glyphBytes, xMin);
            writeI16(glyphBytes, xMax - xMin);
            writeI16(glyphBytes, 0);
            writeI16(glyphBytes, xMin - xMax);

            writeI16(glyphBytes, yMin);
            writeI16(glyphBytes, 0);
            writeI16(glyphBytes, yMax - yMin);
            writeI16(glyphBytes, 0);

            padToFour(glyphBytes);

            return glyphBytes;
        }

        [[nodiscard]] Bytes glyfTable()
        {
            Bytes tableBytes = rectangleGlyph(110, 10, 890, 710);
            const Bytes secondBytes = rectangleGlyph(10, 10, 410, 410);

            tableBytes.insert(
                tableBytes.end(),
                secondBytes.begin(),
                secondBytes.end());

            return tableBytes;
        }

        [[nodiscard]] Bytes locaTable()
        {
            const auto first = static_cast<std::uint32_t>(
                rectangleGlyph(110, 10, 890, 710).size());
            const auto secondBytes = static_cast<std::uint32_t>(
                rectangleGlyph(10, 10, 410, 410).size());

            Bytes tableBytes;

            writeU32(tableBytes, 0);
            writeU32(tableBytes, 0);
            writeU32(tableBytes, 0);
            writeU32(tableBytes, first);
            writeU32(tableBytes, first + secondBytes);

            return tableBytes;
        }

        [[nodiscard]] Bytes cmapTable()
        {
            constexpr std::uint16_t kFirstCode = 0x20;
            constexpr std::uint16_t kEntryCount = 35;

            Bytes tableBytes;

            writeU16(tableBytes, 0);
            writeU16(tableBytes, 1);
            writeU16(tableBytes, 3);
            writeU16(tableBytes, 1);
            writeU32(tableBytes, 12);

            writeU16(tableBytes, 6);
            writeU16(tableBytes, 10 + 2 * kEntryCount);
            writeU16(tableBytes, 0);
            writeU16(tableBytes, kFirstCode);
            writeU16(tableBytes, kEntryCount);

            for (std::uint16_t entry = 0; entry < kEntryCount; ++entry)
            {
                const auto codepoint
                    = static_cast<char32_t>(kFirstCode + entry);

                std::uint16_t glyphBytes = 0;

                if (codepoint == U' ')
                {
                    glyphBytes = 1;
                }
                else if (codepoint == U'A')
                {
                    glyphBytes = 2;
                }
                else if (codepoint == U'B')
                {
                    glyphBytes = 3;
                }

                writeU16(tableBytes, glyphBytes);
            }

            return tableBytes;
        }

        struct Table final
        {
            std::string tag;
            Bytes bytes;
        };
    }

    std::vector<std::uint8_t> buildFont(FontRecipe recipe)
    {
        std::vector<Table> tables;

        tables.push_back({"cmap", cmapTable()});
        tables.push_back({"glyf", glyfTable()});
        tables.push_back({"head", headTable()});
        tables.push_back({"hhea", hheaTable()});
        tables.push_back({"hmtx", hmtxTable()});

        tables.push_back({"loca", locaTable()});
        tables.push_back({"maxp", maxpTable()});

        Bytes fontBytes;

        writeU32(fontBytes, recipe.flavour);
        writeU16(fontBytes, static_cast<std::uint32_t>(tables.size()));
        writeU16(fontBytes, 0);
        writeU16(fontBytes, 0);
        writeU16(fontBytes, 0);

        auto offset = static_cast<std::uint32_t>(
            kOffsetTableSize + kTableRecordSize * tables.size());

        for (const Table &tableBytes : tables)
        {
            writeTag(fontBytes, tableBytes.tag);
            writeU32(fontBytes, 0);
            writeU32(fontBytes, offset);
            writeU32(fontBytes, static_cast<std::uint32_t>(
                tableBytes.bytes.size()));

            offset += static_cast<std::uint32_t>(tableBytes.bytes.size());
        }

        for (const Table &tableBytes : tables)
        {
            fontBytes.insert(
                fontBytes.end(),
                tableBytes.bytes.begin(),
                tableBytes.bytes.end());
        }

        return fontBytes;
    }

    std::vector<std::uint8_t> buildDirectory(
        std::uint32_t flavour,
        std::uint16_t declaredTables,
        std::span<const TableRecord> records,
        std::size_t totalSize)
    {
        Bytes blobBytes;

        writeU32(blobBytes, flavour);
        writeU16(blobBytes, declaredTables);
        writeU16(blobBytes, 0);
        writeU16(blobBytes, 0);
        writeU16(blobBytes, 0);

        for (const TableRecord &record : records)
        {
            writeTag(blobBytes, record.tag);
            writeU32(blobBytes, 0);
            writeU32(blobBytes, record.offset);
            writeU32(blobBytes, record.length);
        }

        blobBytes.resize(totalSize, 0);

        return blobBytes;
    }

}
