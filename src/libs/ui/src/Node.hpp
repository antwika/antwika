#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "FocusRing.hpp"
#include "Interactive.hpp"
#include "NodeKind.hpp"

namespace antwika::ui::detail
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    inline constexpr std::size_t kNoNode =
        std::numeric_limits<std::size_t>::max();

    struct SplitInfo final
    {
        std::uint32_t ratio = 0;

        std::uint32_t minimum = 0;

        std::size_t divider = kNoNode;
    };

    struct Node final
    {
        NodeKind kind = NodeKind::Container;

        Axis axis = Axis::Column;

        Sizing width{};

        Sizing height{};

        Alignment cross = Alignment::Start;

        std::uint32_t padding = 0;

        std::uint32_t gap = 0;

        std::optional<Color> background{};

        WidgetId id = kNoWidget;

        std::optional<Interactive> style{};

        std::optional<FocusRing> focusStyle{};

        std::optional<FocusRing> focusRing{};

        bool pressed = false;

        bool clips = false;

        std::optional<SplitInfo> splitInfo{};

        std::string text{};

        std::uint32_t textScale = 0;

        Color textColor{};

        std::uint32_t overhang = 0;

        bool overlay = false;

        std::size_t overlayAnchor = kNoNode;

        WidgetId optionOwner = kNoWidget;

        std::size_t optionIndex = 0;

        std::size_t parent = kNoNode;
        std::size_t firstChild = kNoNode;
        std::size_t lastChild = kNoNode;
        std::size_t nextSibling = kNoNode;

        Size measured{};

        Rect arranged{};
    };

}
