#pragma once

#include <antwika/gfx/Color.hpp>

#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;

    [[nodiscard]] Color resourceColour(Resource resource) noexcept;

    [[nodiscard]] Color serviceColour(Service service) noexcept;

    inline constexpr Color kFireRiskInk{
        .red = 224, .green = 148, .blue = 78};

    inline constexpr Color kCollapseRiskInk{
        .red = 170, .green = 176, .blue = 188};

    inline constexpr Color kDiseaseRiskInk{
        .red = 214, .green = 120, .blue = 148};

}
