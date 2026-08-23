#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "antwika/ui/Theme.hpp"

namespace antwika::ui
{

    [[nodiscard]] std::size_t getWrapColumns(
        const Theme &theme, std::uint32_t width) noexcept;

    [[nodiscard]] std::vector<std::string_view> getWrapText(
        std::string_view text, std::size_t columns);

}
