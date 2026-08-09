#pragma once

#include <cstddef>

#include <antwika/time/Tick.hpp>

namespace antwika::animation
{

    struct KeyFrame final
    {
        std::size_t index{0};

        time::Tick durationTicks{1};

        [[nodiscard]] bool operator==(
            const KeyFrame &other) const noexcept = default;
    };

}
