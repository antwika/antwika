#pragma once

#include <array>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/map/Settings.hpp>

namespace antwika::editor
{

    enum class ToolPlacement : std::uint8_t
    {
        Shape,
        Lamp,
        Stamp,
        Figure,
        Plate,
        Gate,
        StartOrExit,
    };

    [[nodiscard]] constexpr ToolPlacement lastEnumerator(
        ToolPlacement) noexcept
    {
        return ToolPlacement::StartOrExit;
    }

}
