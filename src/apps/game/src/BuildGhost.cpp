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

        // Nowhere to draw one until the pointer has been seen.
        // And nothing to draw under the bar, which covers the grid.
        // And nothing to preview once the palette has been put down.
        // A click would place nothing then, so a ghost would lie.
        if (!hint.has_value() || coveredByUi || !tool.has_value())
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

        // Visible wherever the block fits on the grid at all.
        // So a refusal is shown rather than the preview vanishing.
        // Whether it would land is the separate question below.
        // Asked of the very predicate the sink places through.
        if (fitsIn(cell, footprint, extent))
        {
            ghost.at = cell;
            ghost.visible = true;
            ghost.valid = canPlace(cell, footprint, extent, paths, built);
        }

        return ghost;
    }

} // namespace antwika::game
