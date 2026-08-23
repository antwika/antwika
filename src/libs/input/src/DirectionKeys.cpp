#include "antwika/input/DirectionKeys.hpp"

#include <algorithm>
#include <array>
#include <span>

namespace antwika::input
{

    namespace
    {
        struct DirectionRow final
        {
            Key pressedKey;
            bool DirectionKeys::*heldFlag;
        };

        constexpr std::array kArrowRows{
            DirectionRow{Key::ArrowUp, &DirectionKeys::north},
            DirectionRow{Key::ArrowDown, &DirectionKeys::south},
            DirectionRow{Key::ArrowLeft, &DirectionKeys::west},
            DirectionRow{Key::ArrowRight, &DirectionKeys::east}};

        constexpr std::array kWasdRows{
            DirectionRow{Key::W, &DirectionKeys::north},
            DirectionRow{Key::S, &DirectionKeys::south},
            DirectionRow{Key::A, &DirectionKeys::west},
            DirectionRow{Key::D, &DirectionKeys::east}};

        void hold(
            DirectionKeys &keys,
            const std::span<const DirectionRow> rows,
            const Key key,
            const bool down) noexcept
        {
            const auto foundRow =
                std::ranges::find(rows, key, &DirectionRow::pressedKey);

            if (foundRow == rows.end())
            {
                return;
            }

            keys.*(foundRow->heldFlag) = down;
        }
    }

    void applyArrowKey(
        DirectionKeys &keys,
        const Key key,
        const bool down) noexcept
    {
        hold(keys, kArrowRows, key, down);
    }

    void applyWasdKey(
        DirectionKeys &keys,
        const Key key,
        const bool down) noexcept
    {
        hold(keys, kWasdRows, key, down);
    }

}
