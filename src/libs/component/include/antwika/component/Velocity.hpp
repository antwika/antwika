#pragma once

namespace antwika::component
{

    struct Velocity final
    {
        float velocityX = 0.0F;
        float velocityZ = 0.0F;

        float speedMultiplier = 1.0F;

        [[nodiscard]] bool operator==(
            const Velocity &other) const = default;
    };

}
