#include "antwika/life/LifeSystem.hpp"

#include <cstdint>

#include "antwika/life/Cell.hpp"

namespace antwika::life
{

    LifeSystem::LifeSystem(const Grid &grid) : grid(grid)
    {
    }

    int LifeSystem::countAliveNeighbors(
        const World &world, std::uint32_t x, std::uint32_t y) const
    {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0)
                {
                    continue;
                }

                const auto nx = static_cast<std::int64_t>(x) + dx;
                const auto ny = static_cast<std::int64_t>(y) + dy;
                if (nx < 0 || ny < 0)
                {
                    continue;
                }

                const auto ux = static_cast<std::uint32_t>(nx);
                const auto uy = static_cast<std::uint32_t>(ny);
                if (!grid.contains(ux, uy))
                {
                    continue;
                }

                if (world.get<Cell>(grid.entityAt(ux, uy)).alive)
                {
                    ++count;
                }
            }
        }
        return count;
    }

    void LifeSystem::update(World &world, antwika::time::Tick)
    {
        for (std::uint32_t y = 0; y < grid.height(); ++y)
        {
            for (std::uint32_t x = 0; x < grid.width(); ++x)
            {
                const auto entity = grid.entityAt(x, y);
                const auto neighbors = countAliveNeighbors(world, x, y);
                const auto wasAlive = world.get<Cell>(entity).alive;
                const auto nowAlive = wasAlive
                                           ? (neighbors == 2 || neighbors == 3)
                                           : (neighbors == 3);
                world.set<Cell>(entity, Cell{.alive = nowAlive});
            }
        }
    }

}
