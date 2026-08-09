#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace antwika::font::tests
{

    struct FontRecipe final
    {
        std::uint32_t flavour = 0x00010000;
    };

    [[nodiscard]] std::vector<std::uint8_t> buildFont(
        FontRecipe recipe = {});

    struct TableRecord final
    {
        std::string_view tag;
        std::uint32_t offset = 0;
        std::uint32_t length = 0;
    };

    [[nodiscard]] std::vector<std::uint8_t> buildDirectory(
        std::uint32_t flavour,
        std::uint16_t declaredTables,
        std::span<const TableRecord> records,
        std::size_t totalSize);

}
