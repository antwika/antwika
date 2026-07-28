#include "antwika/life/Board.hpp"

#include "antwika/life/Cell.hpp"

namespace antwika::life
{

    Board readBoard(const World &world, const Grid &grid)
    {
        Board board{
            .width = grid.width(),
            .height = grid.height(),
            .alive = std::vector<bool>(
                static_cast<std::size_t>(grid.width()) * grid.height()),
        };

        for (std::uint32_t y = 0; y < grid.height(); ++y)
        {
            for (std::uint32_t x = 0; x < grid.width(); ++x)
            {
                const auto entity = grid.entityAt(x, y);
                const auto index =
                    static_cast<std::size_t>(y) * board.width + x;
                board.alive[index] = world.get<Cell>(entity).alive;
            }
        }

        return board;
    }

} // namespace antwika::life
