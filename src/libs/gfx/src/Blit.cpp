#include "antwika/gfx/Blit.hpp"

#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    bool blitIsDrawable(
        const Size texture,
        const RectF source,
        const RectF destination) noexcept
    {
        if (source.size.width <= 0.0F || source.size.height <= 0.0F)
        {
            return false;
        }

        if (destination.size.width <= 0.0F
            || destination.size.height <= 0.0F)
        {
            return false;
        }

        if (source.origin.x < 0.0F || source.origin.y < 0.0F)
        {
            return false;
        }

        const auto right = source.origin.x + source.size.width;
        const auto bottom = source.origin.y + source.size.height;

        return right <= static_cast<float>(texture.width)
            && bottom <= static_cast<float>(texture.height);
    }

}
