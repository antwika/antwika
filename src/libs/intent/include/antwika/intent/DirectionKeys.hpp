#pragma once

namespace antwika::intent
{

    struct DirectionKeys final
    {
        bool north = false;
        bool south = false;
        bool west = false;
        bool east = false;

        [[nodiscard]] float axisX() const noexcept;

        [[nodiscard]] float axisZ() const noexcept;

        [[nodiscard]] bool operator==(
            const DirectionKeys &other) const = default;
    };

}
