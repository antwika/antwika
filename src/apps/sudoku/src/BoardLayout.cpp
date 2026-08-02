#include "antwika/sudoku/BoardLayout.hpp"

#include <cstddef>

namespace antwika::sudoku
{

    namespace
    {
        constexpr std::uint32_t kSide =
            static_cast<std::uint32_t>(Board::kSize);
    } // namespace

    std::optional<BoardLayout> layoutFor(const Rect area)
    {
        const auto byWidth = area.size.width / kSide;
        const auto byHeight = area.size.height / kSide;
        const auto cell = byWidth < byHeight ? byWidth : byHeight;

        if (cell == 0)
        {
            return std::nullopt;
        }

        const auto used = cell * kSide;

        return BoardLayout{
            .cell = cell,
            .origin = {
                .x = area.origin.x
                     + static_cast<std::int32_t>(
                         (area.size.width - used) / 2),
                .y = area.origin.y
                     + static_cast<std::int32_t>(
                         (area.size.height - used) / 2)}};
    }

    std::optional<Square> cellAt(
        const BoardLayout &layout,
        const std::int32_t x,
        const std::int32_t y)
    {
        if (layout.cell == 0)
        {
            return std::nullopt;
        }

        // Widened before subtracting, so this cannot overflow.
        // A pointer far outside the area should miss, not wrap.
        const auto localX =
            static_cast<std::int64_t>(x) - layout.origin.x;
        const auto localY =
            static_cast<std::int64_t>(y) - layout.origin.y;

        if (localX < 0 || localY < 0)
        {
            return std::nullopt;
        }

        const auto column =
            static_cast<std::uint64_t>(localX) / layout.cell;
        const auto row = static_cast<std::uint64_t>(localY) / layout.cell;

        if (column >= kSide || row >= kSide)
        {
            return std::nullopt;
        }

        return Square{
            .row = static_cast<std::size_t>(row),
            .col = static_cast<std::size_t>(column)};
    }

    Rect squareRect(const BoardLayout &layout, const Square square)
    {
        return Rect{
            .origin = {
                .x = layout.origin.x
                     + static_cast<std::int32_t>(
                         square.col * layout.cell),
                .y = layout.origin.y
                     + static_cast<std::int32_t>(
                         square.row * layout.cell)},
            .size = {.width = layout.cell, .height = layout.cell}};
    }

} // namespace antwika::sudoku
