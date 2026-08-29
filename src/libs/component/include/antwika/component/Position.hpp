#pragma once

namespace antwika::component
{

    struct Position final
    {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;

        [[nodiscard]] bool operator==(
            const Position &other) const = default;
    };

}
