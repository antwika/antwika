#pragma once

#include <array>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::editor
{

    enum class ToolPlacement : std::uint8_t
    {
        Shape,
        Lamp,
        Stamp,
        Character,
        Marker,
        StartOrExit,
        Select,
    };

    [[nodiscard]] constexpr ToolPlacement getLastEnumerator(
        ToolPlacement) noexcept
    {
        return ToolPlacement::Select;
    }

}
