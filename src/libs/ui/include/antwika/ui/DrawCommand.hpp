#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/command/DrawText.hpp"
#include "antwika/ui/command/DrawTexture.hpp"
#include "antwika/ui/command/FillRect.hpp"
#include "antwika/ui/command/PopClip.hpp"
#include "antwika/ui/command/PushClip.hpp"

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    using DrawCommand = std::variant<
        FillRect,
        DrawText,
        DrawTexture,
        PushClip,
        PopClip>;

}
