#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "antwika/ui/DragOrigin.hpp"
#include "antwika/ui/LineRun.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/TextHighlight.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct TextAreaSpec final
    {
        WidgetId widgetId = kNoWidget;

        Sizing widthSizing = kGrowSizing;

        Sizing heightSizing = kGrowSizing;

        std::string_view text{};

        std::string_view placeholder{};

        std::size_t cursor = kCaretAtEnd;

        std::optional<std::size_t> anchor{};

        std::size_t scroll = 0;

        std::span<const TextHighlight> highlights{};

        std::span<const LineRun> bandRuns{};

        bool scrollbar = false;

        bool focused = false;

        DragOrigin dragging = DragOrigin::None;
    };

}
