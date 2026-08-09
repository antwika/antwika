#pragma once

#include <antwika/gfx/Size.hpp>

#include "antwika/ui/WidgetRects.hpp"

#include "LayoutTree.hpp"

namespace antwika::ui::detail
{

    using antwika::gfx::Size;

    void layout(
        LayoutTree &tree, Size canvas, WidgetRects *rects = nullptr);

}
