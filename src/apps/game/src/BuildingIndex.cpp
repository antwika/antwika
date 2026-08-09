#include "antwika/game/BuildingIndex.hpp"

#include <cstddef>
#include <cstdint>

namespace antwika::game
{

    bool BuildingIndex::insert(Cell origin, Footprint footprint)
    {
        if (!free(origin, footprint))
        {
            return false;
        }

        for (std::int32_t dy = 0; dy < footprint.height; ++dy)
        {
            for (std::int32_t dx = 0; dx < footprint.width; ++dx)
            {
                occupied.insert(
                    Cell{.x = origin.x + dx, .y = origin.y + dy});
            }
        }

        return true;
    }

    bool BuildingIndex::erase(Cell origin, Footprint footprint)
    {
        bool removed = false;

        for (std::int32_t dy = 0; dy < footprint.height; ++dy)
        {
            for (std::int32_t dx = 0; dx < footprint.width; ++dx)
            {
                removed |= occupied.erase(
                               Cell{.x = origin.x + dx, .y = origin.y + dy})
                    > 0;
            }
        }

        return removed;
    }

    bool BuildingIndex::free(Cell origin, Footprint footprint) const
    {
        for (std::int32_t dy = 0; dy < footprint.height; ++dy)
        {
            for (std::int32_t dx = 0; dx < footprint.width; ++dx)
            {
                if (has(Cell{.x = origin.x + dx, .y = origin.y + dy}))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool BuildingIndex::has(Cell cell) const
    {
        return occupied.contains(cell);
    }

    std::size_t BuildingIndex::size() const noexcept
    {
        return occupied.size();
    }

    const std::set<Cell> &BuildingIndex::cells() const noexcept
    {
        return occupied;
    }

}
