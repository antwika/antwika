#include "FontDirectory.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "antwika/font/FontError.hpp"

namespace antwika::font::detail
{

    namespace
    {
        constexpr std::size_t kOffsetTableSize = 12;
        constexpr std::size_t kTableRecordSize = 16;

        constexpr std::uint32_t kTrueTypeTag = 0x00010000;
        constexpr std::uint32_t kAppleTrueTypeTag = 0x74727565;
        constexpr std::uint32_t kCollectionTag = 0x74746366;
        constexpr std::uint32_t kOpenTypeTag = 0x4F54544F;

        [[nodiscard]] std::uint16_t readU16(
            std::span<const std::uint8_t> bytes, std::size_t offset)
        {
            return static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(bytes[offset]) << 8)
                | bytes[offset + 1]);
        }

        [[nodiscard]] std::uint32_t readU32(
            std::span<const std::uint8_t> bytes, std::size_t offset)
        {
            return (static_cast<std::uint32_t>(readU16(bytes, offset)) << 16)
                | readU16(bytes, offset + 2);
        }

        [[nodiscard]] std::string tagText(
            std::span<const std::uint8_t> bytes, std::size_t offset)
        {
            std::string text;

            for (std::size_t index = 0; index < 4; ++index)
            {
                const auto byte = bytes[offset + index];

                text.push_back(
                    byte >= 0x20 && byte < 0x7F
                          ? static_cast<char>(byte)
                          : '?');
            }

            return text;
        } // GCOVR_EXCL_LINE

        void requireFlavour(std::span<const std::uint8_t> bytes)
        {
            const std::uint32_t flavour = readU32(bytes, 0);

            if (flavour == kCollectionTag)
            {
                throw FontError(
                    "font: this is a font collection rather than a font, "
                    "so which font it holds has to be decided before "
                    "the bytes get here");
            }

            if (flavour == kOpenTypeTag)
            {
                throw FontError(
                    "font: this is an OpenType font with CFF outlines, "
                    "which this library does not read");
            }

            if (flavour != kTrueTypeTag
                && flavour != kAppleTrueTypeTag)
            {
                throw FontError(
                    "font: these bytes open with '" + tagText(bytes, 0)
                    + "' rather than a TrueType flavour");
            }
        }

        void requireTablesInside(std::span<const std::uint8_t> bytes)
        {
            const std::uint16_t tables = readU16(bytes, 4);

            if (tables == 0)
            {
                throw FontError("font: the font declares no tables");
            }

            const std::size_t directoryEnd = kOffsetTableSize
                + static_cast<std::size_t>(tables) * kTableRecordSize;

            if (directoryEnd > bytes.size())
            {
                throw FontError(
                    "font: the font declares "
                    + std::to_string(tables)
                    + " tables, whose directory runs past its end");
            }

            for (std::size_t index = 0; index < tables; ++index)
            {
                const std::size_t record = kOffsetTableSize
                    + index * kTableRecordSize;
                const std::size_t offset = readU32(bytes, record + 8);
                const std::size_t length = readU32(bytes, record + 12);

                if (offset > bytes.size()
                    || length > bytes.size() - offset)
                {
                    throw FontError(
                        "font: the '" + tagText(bytes, record)
                        + "' table runs past the end of the font");
                }
            }
        }
    }

    void requireReadableDirectory(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() < kOffsetTableSize)
        {
            throw FontError(
                "font: " + std::to_string(bytes.size())
                + " bytes is too short to hold a font's offset table");
        }

        requireFlavour(bytes);
        requireTablesInside(bytes);
    }

}
