#include "antwika/game/BuildGhost.hpp"

#include <antwika/gfx/Point.hpp>

#include "antwika/game/IsoProjection.hpp"

namespace antwika::game
{

    BuildGhost ghostFor(
        const std::optional<antwika::input::PointerHint> &hint,
        const Camera &camera,
        GridExtent extent,
        BuildTool tool,
        bool coveredByUi) noexcept
    {
        BuildGhost ghost{.at = {}, .tool = tool, .visible = false};

        // Nowhere to draw one until the pointer has been seen.
        // And nothing to draw under the bar, which covers the grid.
        if (!hint.has_value() || coveredByUi)
        {
            return ghost;
        }

        const auto cell = screenToCell(
            antwika::gfx::Point{
                .x = hint->position.x, .y = hint->position.y},
            camera);

        if (extent.contains(cell))
        {
            ghost.at = cell;
            ghost.visible = true;
        }

        return ghost;
    }

} // namespace antwika::game
