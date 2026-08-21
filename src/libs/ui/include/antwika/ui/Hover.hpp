#pragma once

#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/HoverPointer.hpp"
#include "antwika/ui/HoverTargets.hpp"

namespace antwika::ui
{

    void applyHover(
        DrawList &drawList,
        const HoverTargets &targets,
        HoverPointer hover);

}
