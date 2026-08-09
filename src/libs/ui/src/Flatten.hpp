#pragma once

#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/HoverTargets.hpp"

#include "LayoutTree.hpp"

namespace antwika::ui::detail
{

    [[nodiscard]] DrawList flatten(
        const LayoutTree &tree, HoverTargets *targets = nullptr);

}
