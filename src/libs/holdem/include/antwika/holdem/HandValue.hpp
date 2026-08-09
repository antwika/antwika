#pragma once

#include <cstdint>

#include "antwika/holdem/HandCategory.hpp"

namespace antwika::holdem
{

    enum class HandValue : std::uint32_t
    {
    };

    inline constexpr unsigned kSlotBits = 4;

    inline constexpr unsigned kSlotCount = 5;

    inline constexpr unsigned kCategoryShift = kSlotBits * kSlotCount;

    [[nodiscard]] constexpr std::uint32_t rawValue(HandValue value) noexcept
    {
        return static_cast<std::uint32_t>(value);
    }

    [[nodiscard]] constexpr HandCategory categoryOf(HandValue value) noexcept
    {
        return static_cast<HandCategory>(rawValue(value) >> kCategoryShift);
    }

}
