#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "antwika/ui/DragOrigin.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    struct Area final
    {
        WidgetId widgetId = kNoWidget;

        std::size_t column = 0;

        std::size_t track = 0;

        std::size_t thumb = 0;

        std::string_view text{};

        std::size_t scroll = 0;

        std::size_t requestedExtent = 0;

        std::size_t lines = 1;

        std::size_t cursor = 0;

        std::size_t anchor = 0;

        DragOrigin dragging = DragOrigin::None;

        std::span<const LineRun> bandRuns{};

        std::uint32_t lineHeight = 1;

        std::uint32_t advance = 1;

        bool focused = false;
    };

}
