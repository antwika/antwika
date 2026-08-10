#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/Entities.hpp>

namespace antwika::map_editor
{

    enum class MarkerKind : std::uint8_t
    {
        Transition = 0,
        Boat,
        Spawn,
        Pickup,
        Npc,
        Trigger,
    };

    inline constexpr std::size_t kMarkerKindCount = 6;

    inline constexpr std::array<std::string_view, kMarkerKindCount>
        kMarkerKindNames{
            "transition",
            "boat",
            "spawn",
            "pickup",
            "npc",
            "trigger"};

    struct Marker final
    {
        MarkerKind kind = MarkerKind::Transition;
        std::uint32_t index = 0;
    };

    struct CellRef final
    {
        std::uint32_t column = 0;
        std::uint32_t row = 0;
    };

    [[nodiscard]] MarkerKind markerKindOf(
        const tilemap::Entity &entity) noexcept;

    [[nodiscard]] geometry::GridCell entityCellOf(
        const tilemap::Entity &entity) noexcept;

}
