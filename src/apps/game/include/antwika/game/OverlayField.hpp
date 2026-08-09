#pragma once

#include <cstdint>
#include <map>

#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Color.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/MapView.hpp"

namespace antwika::game
{

    using antwika::ecs::World;
    using antwika::gfx::Color;

    using OverlayField = std::map<Cell, std::int32_t>;

    [[nodiscard]] OverlayField overlayFieldOf(
        const World &world,
        MapView view,
        const DesirabilityField &desirability,
        GridExtent extent);

    [[nodiscard]] Color overlayColour(MapView view) noexcept;

    inline constexpr Color kOverlayScrim{
        .red = 8, .green = 10, .blue = 16, .alpha = 175};

    inline constexpr std::uint8_t kOverlayFaintest = 60;

}
