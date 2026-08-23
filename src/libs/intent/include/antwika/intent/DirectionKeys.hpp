#pragma once

namespace antwika::intent
{

    struct DirectionKeys final
    {
        bool north = false;
        bool south = false;
        bool west = false;
        bool east = false;

        [[nodiscard]] float getAxisX() const noexcept;

        [[nodiscard]] float getAxisZ() const noexcept;

        [[nodiscard]] bool operator==(
            const DirectionKeys &other) const = default;
    };

}
