#pragma once

#include <optional>

#include "antwika/tilemap/FlowDirection.hpp"

namespace antwika::tilemap
{

    struct WaterAttributes final
    {
        bool deadly = false;
        bool swimmable = false;
        std::optional<FlowDirection> current{};

        [[nodiscard]] bool operator==(const WaterAttributes &other) const
            = default;
    };

}
