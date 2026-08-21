#pragma once

#include <antwika/input/Key.hpp>

namespace antwika::input
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
            const DirectionKeys &other) const
            = default;
    };

    void applyArrowKey(
        DirectionKeys &keys, Key key, bool down) noexcept;

    void applyWasdKey(
        DirectionKeys &keys, Key key, bool down) noexcept;

}
