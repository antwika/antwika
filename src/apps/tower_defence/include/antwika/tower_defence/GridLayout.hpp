#pragma once

#include <cstdint>
#include <optional>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/tower_defence/Level.hpp"

namespace antwika::tower_defence
{

    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    using GridLayout = antwika::geometry::Grid;

    [[nodiscard]] std::uint32_t scoreBarHeight(Size canvas) noexcept;

    [[nodiscard]] std::optional<GridLayout> layoutFor(
        Size canvas, std::uint32_t width, std::uint32_t height);

    [[nodiscard]] std::optional<Cell> cellAt(
        const GridLayout &layout, std::int32_t x, std::int32_t y);

    [[nodiscard]] Rect cellRect(const GridLayout &layout, const Cell &cell);

}
