#pragma once

#include <cstddef>
#include <cstdint>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include "FocusRing.hpp"
#include "NodeKind.hpp"
#include "StateColors.hpp"
#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Icon.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    inline constexpr std::size_t kNoNode =
        std::numeric_limits<std::size_t>::max();

    struct SplitInfo final
    {
        std::uint32_t ratio = 0;

        std::uint32_t minimum = 0;

        std::size_t divider = kNoNode;
    };

}
