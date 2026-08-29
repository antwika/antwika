#pragma once

namespace antwika::component
{

    struct DirectionKeys final
    {
        bool north = false;
        bool south = false;
        bool west = false;
        bool east = false;

        [[nodiscard]] float getAxisX() const noexcept
        {
            return (east ? 1.0F : 0.0F) - (west ? 1.0F : 0.0F);
        }

        [[nodiscard]] float getAxisZ() const noexcept
        {
            return (south ? 1.0F : 0.0F) - (north ? 1.0F : 0.0F);
        }

        [[nodiscard]] bool operator==(
            const DirectionKeys &other) const = default;
    };

}
