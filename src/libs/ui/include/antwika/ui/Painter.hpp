#pragma once

#include <antwika/gfx/ISurfaceRenderer.hpp>

#include "antwika/ui/DrawList.hpp"

namespace antwika::ui
{

    using antwika::gfx::ISurfaceRenderer;

    void paint(ISurfaceRenderer &renderer, const DrawList &drawList);

}
