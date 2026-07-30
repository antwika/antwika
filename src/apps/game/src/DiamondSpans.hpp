#pragma once

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Point.hpp>

namespace antwika::game::detail
{

    using antwika::gfx::Color;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Point;

    /**
     * @brief Fill an isometric diamond, one horizontal line per row.
     *
     * A diamond has no axis-aligned edges, so it is filled as a stack of
     * lines whose widths step linearly from nothing at the top corner, to
     * the full half-width at the middle, back to nothing at the bottom.
     *
     * The top and bottom rows are zero-width lines, which is why
     * IRenderer::drawLine promises both endpoints: a backend dropping one
     * would leave the diamond's points missing.
     *
     * Private to the scene, next to its only caller. Every offset is
     * computed in signed arithmetic, because the unsigned subtraction the
     * obvious version invites underflows into a four-billion-pixel line
     * somewhere else on the screen rather than a visibly wrong diamond.
     *
     * @param renderer Receives one drawLine per row.
     * @param centre The diamond's middle.
     * @param halfWidth Pixels from the middle to the left or right corner.
     * @param halfHeight Pixels from the middle to the top or bottom
     * corner; zero draws a single line through the middle.
     * @param color The colour to fill with.
     */
    void fillDiamond(
        IRenderer &renderer,
        Point centre,
        std::int32_t halfWidth,
        std::int32_t halfHeight,
        Color color);

} // namespace antwika::game::detail
