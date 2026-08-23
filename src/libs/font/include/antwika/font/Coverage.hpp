#pragma once

#include <cstdint>
#include <vector>

namespace antwika::font
{

    struct Coverage final
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<std::uint8_t> samples;

        [[nodiscard]] bool isValid() const noexcept;

        [[nodiscard]] std::uint8_t getEntryAt(
            std::uint32_t x, std::uint32_t y) const;

        [[nodiscard]] bool operator==(const Coverage &other) const
            = default;
    };

}
