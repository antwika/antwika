#pragma once

#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    [[nodiscard]] bool blitIsDrawable(
        Size texture, RectF source, RectF destination) noexcept;

}
