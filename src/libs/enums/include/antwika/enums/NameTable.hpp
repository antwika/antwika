#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

#include "antwika/enums/Enumeration.hpp"

namespace antwika::enums
{

    template <typename Enum>
    struct NameTable final
    {
        std::array<std::string_view, kCount<Enum>> names;

        [[nodiscard]] constexpr std::string_view name(
            const Enum value) const noexcept
        {
            return lookup(names, value);
        }

        [[nodiscard]] constexpr std::optional<Enum> from(
            const std::string_view name) const noexcept
        {
            for (std::size_t index = 0; index < kCount<Enum>; ++index)
            {
                if (names[index] == name)
                {
                    return static_cast<Enum>(index);
                }
            }

            return std::nullopt;
        }
    };

}
