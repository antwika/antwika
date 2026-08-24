#pragma once

#include <cstdint>

#include <antwika/gfx/Math3D.hpp>

namespace antwika::map
{

    struct Placement final
    {
        gfx::Vec3 position{};

        std::uint8_t way = 0;

        [[nodiscard]] bool operator==(const Placement &other) const
            = default;
    };

}
