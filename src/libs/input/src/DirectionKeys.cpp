#include "antwika/input/DirectionKeys.hpp"

namespace antwika::input
{

    namespace
    {
        void hold(
            DirectionKeys &keys,
            const Key key,
            const bool down,
            const Key northKey,
            const Key southKey,
            const Key westKey,
            const Key eastKey) noexcept
        {
            if (key == northKey)
            {
                keys.north = down;
            }
            else if (key == southKey)
            {
                keys.south = down;
            }
            else if (key == westKey)
            {
                keys.west = down;
            }
            else if (key == eastKey)
            {
                keys.east = down;
            }
        }
    }

    float DirectionKeys::axisX() const noexcept
    {
        return (east ? 1.0F : 0.0F) - (west ? 1.0F : 0.0F);
    }

    float DirectionKeys::axisZ() const noexcept
    {
        return (south ? 1.0F : 0.0F) - (north ? 1.0F : 0.0F);
    }

    void applyArrowKey(
        DirectionKeys &keys,
        const Key key,
        const bool down) noexcept
    {
        hold(
            keys,
            key,
            down,
            Key::ArrowUp,
            Key::ArrowDown,
            Key::ArrowLeft,
            Key::ArrowRight);
    }

    void applyWasdKey(
        DirectionKeys &keys,
        const Key key,
        const bool down) noexcept
    {
        hold(
            keys,
            key,
            down,
            Key::W,
            Key::S,
            Key::A,
            Key::D);
    }

}
