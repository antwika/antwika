#include "antwika/game/OverlayLabel.hpp"

#include <algorithm>

#include <antwika/gfx/TextLayout.hpp>

#include "antwika/game/IsoProjection.hpp"

namespace antwika::game
{

    namespace
    {
        [[nodiscard]] std::uint32_t scaleFor(const Camera &camera) noexcept
        {
            return std::max(
                std::uint32_t{1},
                tileSize(camera).width / kTileWidthPerTextScale);
        }
    }

    std::optional<OverlayLabel> overlayLabelFor(
        std::string_view text, Cell cell, const Camera &camera)
    {
        const auto scale = scaleFor(camera);
        const auto written = antwika::gfx::textSize(text, scale);
        const auto tile = tileSize(camera);

        if (written.width > tile.width || written.height > tile.height)
        {
            return std::nullopt;
        }

        const auto centre = cellCentre(cell, camera);

        return OverlayLabel{
            .origin =
                Point{
                    .x = centre.x
                        - static_cast<std::int32_t>(written.width / 2),
                    .y = centre.y
                        - static_cast<std::int32_t>(written.height / 2)},
            .scale = scale};
    }

}
