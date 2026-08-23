#pragma once

namespace antwika::component
{

    struct TurnIntent final
    {
        float axisX = 0.0F;

        float axisZ = 0.0F;

        [[nodiscard]] bool operator==(
            const TurnIntent &other) const = default;
    };

}
