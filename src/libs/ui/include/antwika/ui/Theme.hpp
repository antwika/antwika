#pragma once

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Size;
    using antwika::gfx::TextFace;

    struct Theme final
    {
        Color panelColor{.red = 28, .green = 30, .blue = 38};

        Color textColor{.red = 232, .green = 236, .blue = 232};

        Color mutedColor{.red = 120, .green = 140, .blue = 128};

        Color buttonIdleColor{.red = 48, .green = 52, .blue = 64};

        Color buttonHoveredColor{.red = 68, .green = 74, .blue = 92};

        Color buttonPressedColor{.red = 32, .green = 36, .blue = 44};

        Color buttonTextColor{.red = 240, .green = 240, .blue = 240};

        Color fieldColor{.red = 20, .green = 22, .blue = 28};

        Color fieldFocusedColor{.red = 14, .green = 16, .blue = 20};

        Color caretColor{.red = 232, .green = 236, .blue = 232};

        Color selectionColor{.red = 44, .green = 72, .blue = 116};

        Color highlightColor{.red = 38, .green = 84, .blue = 52};

        Color scrollTrackColor{.red = 30, .green = 33, .blue = 42};

        Color scrollThumbColor{.red = 78, .green = 86, .blue = 106};

        Color focusRingColor{.red = 244, .green = 208, .blue = 63};

        Color dividerColor{.red = 42, .green = 46, .blue = 58};

        Color dividerHoveredColor{.red = 70, .green = 78, .blue = 96};

        Color dividerHeldColor{.red = 96, .green = 106, .blue = 130};

        TextFace face = TextFace::Normal;

        std::uint32_t textScale = 1;

        std::uint32_t padding = 4;

        std::uint32_t gap = 4;

        std::uint32_t buttonPadding = 6;

        std::uint32_t checkboxSize = 7;

        std::uint32_t checkboxInset = 2;

        std::uint32_t focusRingThickness = 1;

        std::uint32_t scrollbarWidth = 8;

        std::uint32_t sliderHeight = 12;

        std::uint32_t sliderThumbWidth = 8;

        std::uint32_t dividerThickness = 6;
    };

    [[nodiscard]] std::uint32_t scaleForCanvas(Size canvasSize) noexcept;

    [[nodiscard]] Theme scaledTheme(
        Theme baseTheme, std::uint32_t scale) noexcept;

}
