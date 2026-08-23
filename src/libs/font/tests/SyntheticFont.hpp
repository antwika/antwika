#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "FontRecipe.hpp"
#include "TableRecord.hpp"

namespace antwika::font::tests
{

    [[nodiscard]] std::vector<std::uint8_t> createFont(
        FontRecipe recipe = {});

    [[nodiscard]] std::vector<std::uint8_t> createDirectory(
        std::uint32_t flavour,
        std::uint16_t declaredTables,
        std::span<const TableRecord> records,
        std::size_t totalSize);

}
