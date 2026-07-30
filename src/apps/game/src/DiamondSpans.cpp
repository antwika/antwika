#include "DiamondSpans.hpp"

namespace antwika::game::detail
{

    void fillDiamond(
        IRenderer &renderer,
        Point centre,
        std::int32_t halfWidth,
        std::int32_t halfHeight,
        Color color)
    {
        // A diamond with no height is the one line through its middle.
        // Dividing by its height to taper the rows would divide by zero.
        if (halfHeight <= 0)
        {
            renderer.drawLine(
                Point{.x = centre.x - halfWidth, .y = centre.y},
                Point{.x = centre.x + halfWidth, .y = centre.y},
                color);
            return;
        }

        for (std::int32_t row = -halfHeight; row <= halfHeight; ++row)
        {
            const auto rise = row < 0 ? -row : row;
            const auto width = halfWidth * (halfHeight - rise) / halfHeight;

            renderer.drawLine(
                Point{.x = centre.x - width, .y = centre.y + row},
                Point{.x = centre.x + width, .y = centre.y + row},
                color);
        }
    }

} // namespace antwika::game::detail
