#pragma once

#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    [[nodiscard]] bool blitIsDrawable(
        Size texture, Rect source, Rect destination) noexcept;

}
