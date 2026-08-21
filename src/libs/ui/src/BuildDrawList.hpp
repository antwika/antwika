#pragma once

#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/HoverTargets.hpp"

#include "LayoutTree.hpp"

namespace antwika::ui::detail
{

    [[nodiscard]] DrawList buildDrawList(
        const LayoutTree &tree, HoverTargets *targets = nullptr);

}
