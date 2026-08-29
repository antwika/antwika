#include "antwika/editor/editor/state/PanelSizes.hpp"

#include <algorithm>
#include <cstdint>

#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>

namespace antwika::editor
{

    std::uint32_t getFittedPanelWidth(
        const std::uint32_t wish,
        const std::uint32_t restingWidth,
        const std::uint32_t windowWidth) noexcept
    {
        if (wish == 0)
        {
            return restingWidth;
        }

        const auto ceiling = std::max(windowWidth / 3, kMinPanelWidth);

        return std::clamp(wish, kMinPanelWidth, ceiling);
    }

    float getRailWidthOnCanvas(
        const PanelSizes &panelSizes,
        const gfx::Size windowSize,
        const gfx::Size canvasSize) noexcept
    {
        if (panelSizes.railWidth == 0 || windowSize.width == 0)
        {
            return kRightPanelWidth;
        }

        return static_cast<float>(
                   getFittedPanelWidth(
                       panelSizes.railWidth,
                       getRailWidth(windowSize, canvasSize),
                       windowSize.width))
               * static_cast<float>(canvasSize.width)
               / static_cast<float>(windowSize.width);
    }

}
