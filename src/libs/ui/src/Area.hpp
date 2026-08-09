#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    struct Area final
    {
        WidgetId id = kNoWidget;

        std::size_t column = 0;

        std::size_t track = 0;

        std::size_t thumb = 0;

        std::string_view text{};

        std::size_t scroll = 0;

        std::size_t requested = 0;

        std::size_t lines = 1;

        std::size_t cursor = 0;

        std::size_t anchor = 0;

        DragHome dragging = DragHome::None;

        std::span<const LineBand> bands{};

        std::uint32_t lineHeight = 1;

        std::uint32_t advance = 1;

        bool focused = false;
    };

}
