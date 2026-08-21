#pragma once

#include <cstdint>

#include <antwika/gfx/PointF.hpp>

namespace antwika::ui
{

    struct ClickTrack final
    {
        std::uint32_t lastFrame = 0;

        gfx::PointF lastPoint{};

        bool hasClick = false;

        [[nodiscard]] bool operator==(const ClickTrack &other) const
            = default;
    };

}
