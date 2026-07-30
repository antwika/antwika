#include "antwika/gfx/Blit.hpp"

#include <cstdint>

#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    bool blitIsDrawable(
        Size texture, Rect source, Rect destination) noexcept
    {
        if (source.size.width == 0 || source.size.height == 0)
        {
            return false;
        }

        if (destination.size.width == 0 || destination.size.height == 0)
        {
            return false;
        }

        if (source.origin.x < 0 || source.origin.y < 0)
        {
            return false;
        }

        // Widened because an origin is signed and a size is not.
        // A source near the top of the range would otherwise wrap in.
        const auto right = static_cast<std::int64_t>(source.origin.x)
            + static_cast<std::int64_t>(source.size.width);
        const auto bottom = static_cast<std::int64_t>(source.origin.y)
            + static_cast<std::int64_t>(source.size.height);

        return right <= static_cast<std::int64_t>(texture.width)
            && bottom <= static_cast<std::int64_t>(texture.height);
    }

} // namespace antwika::gfx
