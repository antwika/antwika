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
        if (!hint.has_value() || coveredByUi)
        {
            return HoverReadout{};
        }

        const antwika::gfx::Point at{
            .x = hint->position.x, .y = hint->position.y};

        const auto cell = screenToCell(at, camera);

        for (const auto &walker : snapshot.walkers)
        {
            if (walker.at == cell)
            {
                return HoverReadout{.anchor = at, .walker = walker};
            }
        }

        for (const auto &building : snapshot.buildings)
        {
            if (covers(building.at, footprintOf(building.kind), cell))
            {
                return HoverReadout{.anchor = at, .building = building};
            }
        }

        for (const auto &ruin : snapshot.ruins)
        {
            if (covers(ruin.at, footprintOf(ruin.kind), cell))
            {
                return HoverReadout{.anchor = at, .ruin = ruin};
            }
        }

        return HoverReadout{};
    }

}
