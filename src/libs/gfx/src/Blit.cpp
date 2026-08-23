#include "antwika/gfx/Blit.hpp"

#include <cmath>

#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    bool isBlitIsInBounds(
        const Size textureSize,
        const RectF sourceRect,
        const RectF destinationRect) noexcept
    {
        if (sourceRect.size.width == 0.0F || sourceRect.size.height == 0.0F)
        {
            return false;
        }

        if (destinationRect.size.width <= 0.0F
            || destinationRect.size.height <= 0.0F)
        {
            return false;
        }

        if (sourceRect.originPoint.x < 0.0F || sourceRect.originPoint.y < 0.0F)
        {
            return false;
        }

        const auto right =
            sourceRect.originPoint.x + std::abs(sourceRect.size.width);
        const auto bottom =
            sourceRect.originPoint.y + std::abs(sourceRect.size.height);

        return right <= static_cast<float>(textureSize.width)
            && bottom <= static_cast<float>(textureSize.height);
    }

}
