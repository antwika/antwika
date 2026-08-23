#pragma once

#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    [[nodiscard]] bool isBlitIsInBounds(
        Size textureSize, RectF sourceRect, RectF destinationRect) noexcept;

}
