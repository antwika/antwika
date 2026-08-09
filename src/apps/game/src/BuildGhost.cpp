#include "antwika/game/BuildGhost.hpp"

#include <antwika/gfx/Point.hpp>

#include "antwika/game/Footprint.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Placement.hpp"

namespace antwika::game
{

    BuildGhost ghostFor(
        const std::optional<antwika::input::PointerHint> &hint,
        const Camera &camera,
        GridExtent extent,
        std::optional<BuildTool> tool,
        bool coveredByUi,
        const PathIndex &paths,
        const BuildingIndex &built)
    {
        BuildGhost ghost{
            .at = {},
            .tool = tool.value_or(BuildTool::Road),
            .visible = false,
            .valid = false};

        if (!hint.has_value() || coveredByUi || !tool.has_value())
        {
            return ghost;
        }

        if (*tool == BuildTool::Raze)
        {
            return ghost;
        }

        const auto cell = screenToCell(
            antwika::gfx::Point{
                .x = hint->position.x, .y = hint->position.y},
            camera);

        const auto kind = buildingKindOf(*tool);
        const auto footprint =
            kind.has_value() ? footprintOf(*kind) : Footprint{};

        if (fitsIn(cell, footprint, extent))
        {
            ghost.at = cell;
            ghost.visible = true;
            ghost.valid = canPlace(cell, footprint, extent, paths, built);
        }

        return ghost;
    }

}
