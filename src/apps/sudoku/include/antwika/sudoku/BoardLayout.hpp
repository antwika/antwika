#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/sudoku/Board.hpp"

namespace antwika::sudoku
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    struct BoardLayout final
    {
        std::uint32_t cell = 0;
        Point origin{};

        [[nodiscard]] bool operator==(const BoardLayout &other) const
            = default;
    };

    [[nodiscard]] std::optional<BoardLayout> layoutFor(Rect area);

    [[nodiscard]] std::optional<Square> cellAt(
        const BoardLayout &layout, std::int32_t x, std::int32_t y);

    [[nodiscard]] Rect squareRect(
        const BoardLayout &layout, Square square);

}
