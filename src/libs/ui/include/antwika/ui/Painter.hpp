#pragma once

#include <antwika/gfx/IRenderer.hpp>

#include "antwika/ui/DrawList.hpp"

namespace antwika::ui
{

    using antwika::gfx::IRenderer;

    void paint(IRenderer &renderer, const DrawList &drawList);

}
