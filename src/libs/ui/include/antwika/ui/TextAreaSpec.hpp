#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct TextHighlight final
    {
        std::size_t begin = 0;

        std::size_t end = 0;

        [[nodiscard]] bool operator==(const TextHighlight &other) const
            = default;
    };

    struct LineBand final
    {
        std::size_t line = 0;

        std::uint32_t rows = 0;

        WidgetId id = kNoWidget;

        [[nodiscard]] bool operator==(const LineBand &other) const
            = default;
    };

    enum class DragHome : std::uint8_t
    {
        None = 0,

        Text,

        Track,
    };

    [[nodiscard]] constexpr DragHome enumBound(DragHome) noexcept
    {
        return DragHome::Track;
    }

    struct TextAreaSpec final
    {
        WidgetId id = kNoWidget;

        Sizing width = kGrow;

        Sizing height = kGrow;

        std::string_view text{};

        std::string_view placeholder{};

        std::size_t cursor = kCaretAtEnd;

        std::optional<std::size_t> anchor{};

        std::size_t scroll = 0;

        std::span<const TextHighlight> highlights{};

        std::span<const LineBand> bands{};

        bool scrollbar = false;

        bool focused = false;

        DragHome dragging = DragHome::None;
    };

}
