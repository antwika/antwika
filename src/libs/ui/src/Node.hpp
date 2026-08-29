#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "FocusRing.hpp"
#include "NodeKind.hpp"
#include "SplitInfo.hpp"
#include "StateColors.hpp"
#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Icon.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;
    using antwika::gfx::TextScale;

    struct Node final
    {
        NodeKind kind = NodeKind::Container;

        Axis axis = Axis::Column;

        Sizing widthSizing{};

        Sizing heightSizing{};

        Alignment crossAlignment = Alignment::Start;

        std::uint32_t padding = 0;

        std::uint32_t gap = 0;

        std::optional<Color> backgroundColor{};

        WidgetId widgetId = kNoWidget;

        std::optional<StateColors> styleColors{};

        std::optional<FocusRing> focusStyle{};

        std::optional<FocusRing> focusRing{};

        bool pressed = false;

        bool clips = false;

        std::optional<std::uint32_t> scrollOffset{};

        std::optional<SplitInfo> splitInfo{};

        std::string text{};

        TextScale textScale{};

        Color textColor{};

        Icon imageIcon{};

        Color tintColor{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        std::uint32_t extraWidth = 0;

        bool overlay = false;

        std::size_t overlayAnchor = kNoNode;

        WidgetId optionOwnerWidget = kNoWidget;

        std::size_t optionIndex = 0;

        std::size_t parent = kNoNode;
        std::size_t firstChild = kNoNode;
        std::size_t lastChild = kNoNode;
        std::size_t nextSibling = kNoNode;

        Size measuredSize{};

        Rect arrangedRect{};
    };

}
