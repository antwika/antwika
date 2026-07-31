#include "antwika/game/Placement.hpp"

#include <cstdint>

namespace antwika::game
{

    bool canPlace(
        Cell origin,
        Footprint footprint,
        GridExtent extent,
        const PathIndex &paths,
        const BuildingIndex &built)
    {
        if (!fitsIn(origin, footprint, extent))
        {
            return false;
        }

        if (!built.free(origin, footprint))
        {
            return false;
        }

        // Asked cell by cell rather than of the whole block.
        // PathIndex answers about one cell at a time.
        // And a road anywhere under the block refuses it.
        for (std::int32_t dy = 0; dy < footprint.height; ++dy)
        {
            for (std::int32_t dx = 0; dx < footprint.width; ++dx)
            {
                if (paths.has(Cell{.x = origin.x + dx, .y = origin.y + dy}))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool canPave(
        Cell cell,
        GridExtent extent,
        const PathIndex &paths,
        const BuildingIndex &built)
    {
        return canPlace(cell, Footprint{}, extent, paths, built);
    }

} // namespace antwika::game
