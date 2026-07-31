#include "antwika/game/Hover.hpp"

#include <antwika/gfx/Point.hpp>

#include "antwika/game/Footprint.hpp"
#include "antwika/game/IsoProjection.hpp"

namespace antwika::game
{

    HoverReadout hoverFor(
        const std::optional<antwika::input::PointerHint> &hint,
        const Camera &camera,
        const SceneSnapshot &snapshot,
        bool coveredByUi)
    {
        // Nothing to report until the pointer has been seen at all.
        // And nothing under the bar, which covers the grid.
        if (!hint.has_value() || coveredByUi)
        {
            return HoverReadout{};
        }

        const antwika::gfx::Point at{
            .x = hint->position.x, .y = hint->position.y};

        const auto cell = screenToCell(at, camera);

        // The walkers first, since a walker is drawn over a building.
        for (const auto &walker : snapshot.walkers)
        {
            if (walker.at == cell)
            {
                return HoverReadout{.anchor = at, .walker = walker};
            }
        }

        // At most one block can ever cover a cell.
        // BuildingIndex refuses a block overlapping one already there.
        // So there is no order to decide between two answers here.
        for (const auto &building : snapshot.buildings)
        {
            if (covers(building.at, footprintOf(building.kind), cell))
            {
                return HoverReadout{.anchor = at, .building = building};
            }
        }

        return HoverReadout{};
    }

} // namespace antwika::game
