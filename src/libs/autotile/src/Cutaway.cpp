#include "antwika/autotile/Cutaway.hpp"

#include <cstddef>
#include <deque>

namespace antwika::autotile
{

    namespace
    {
        using geometry::GridCell;
        using tilemap::TileMap;

        [[nodiscard]] std::size_t indexOf(
            const TileMap &map, const GridCell cell)
        {
            return static_cast<std::size_t>(cell.row) * map.columns()
                   + cell.column;
        }

        [[nodiscard]] bool rises(
            const TileMap &map,
            const GridCell cell,
            const std::int32_t playerHeight)
        {
            const auto *top = map.at(cell).top();

            return top != nullptr && top->level > playerHeight;
        }
    }

    std::vector<bool> cutawayHidden(
        const TileMap &map,
        const GridCell player,
        const std::int32_t playerHeight)
    {
        std::vector<bool> hidden(
            static_cast<std::size_t>(map.columns()) * map.rows(),
            false);

        std::deque<GridCell> pending;

        const auto seed = [&](const GridCell cell)
        {
            if (cell.column >= map.columns() || cell.row >= map.rows())
            {
                return;
            }

            if (hidden[indexOf(map, cell)]
                || !rises(map, cell, playerHeight))
            {
                return;
            }

            hidden[indexOf(map, cell)] = true;
            pending.push_back(cell);
        };

        seed(player);

        for (std::uint32_t rowStep = 1; rowStep <= 2; ++rowStep)
        {
            for (std::int64_t offset = -1; offset <= 1; ++offset)
            {
                const auto column =
                    static_cast<std::int64_t>(player.column) + offset;

                if (column < 0)
                {
                    continue;
                }

                seed(GridCell{
                    .column = static_cast<std::uint32_t>(column),
                    .row = player.row + rowStep});
            }
        }

        while (!pending.empty())
        {
            const auto cell = pending.front();
            pending.pop_front();

            seed(GridCell{.column = cell.column + 1, .row = cell.row});
            seed(GridCell{.column = cell.column, .row = cell.row + 1});

            if (cell.column > 0)
            {
                seed(GridCell{
                    .column = cell.column - 1, .row = cell.row});
            }

            if (cell.row > 0)
            {
                seed(GridCell{
                    .column = cell.column, .row = cell.row - 1});
            }
        }

        return hidden;
    }

}
