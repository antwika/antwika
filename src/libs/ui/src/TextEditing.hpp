#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    struct TextEditInput final
    {
        WidgetId widgetId = kNoWidget;

        std::string_view text{};

        std::size_t cursor = 0;

        std::size_t anchor = 0;

        bool multiline = false;
    };

    [[nodiscard]] std::size_t beginOfLine(
        std::string_view text, std::size_t charIndex) noexcept;

    [[nodiscard]] std::size_t endOfLine(
        std::string_view text, std::size_t charIndex) noexcept;

    [[nodiscard]] std::optional<TextEdit> editFor(
        const TextEditInput &fieldInput, const Keyboard &keyboard);

}
