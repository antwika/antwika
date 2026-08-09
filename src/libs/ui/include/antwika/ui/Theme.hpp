#pragma once

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Size;

    struct Theme final
    {
        Color panel{.red = 28, .green = 30, .blue = 38};

        Color text{.red = 232, .green = 236, .blue = 232};

        Color muted{.red = 120, .green = 140, .blue = 128};

        Color buttonIdle{.red = 48, .green = 52, .blue = 64};

        Color buttonHovered{.red = 68, .green = 74, .blue = 92};

        Color buttonPressed{.red = 32, .green = 36, .blue = 44};

        Color buttonText{.red = 240, .green = 240, .blue = 240};

        Color field{.red = 20, .green = 22, .blue = 28};

        Color fieldFocused{.red = 14, .green = 16, .blue = 20};

        Color caret{.red = 232, .green = 236, .blue = 232};

        Color selection{.red = 44, .green = 72, .blue = 116};

        Color highlight{.red = 38, .green = 84, .blue = 52};

        Color scrollTrack{.red = 30, .green = 33, .blue = 42};

        Color scrollThumb{.red = 78, .green = 86, .blue = 106};

        Color focusRing{.red = 244, .green = 208, .blue = 63};

        Color divider{.red = 42, .green = 46, .blue = 58};

        Color dividerHovered{.red = 70, .green = 78, .blue = 96};

        Color dividerHeld{.red = 96, .green = 106, .blue = 130};

        std::uint32_t textScale = 1;

        std::uint32_t padding = 4;

        std::uint32_t gap = 4;

        std::uint32_t buttonPadding = 6;

        std::uint32_t focusRingThickness = 2;

        std::uint32_t scrollbarWidth = 8;

        std::uint32_t sliderHeight = 12;

        std::uint32_t sliderThumbWidth = 8;

        std::uint32_t dividerThickness = 6;
    };

    [[nodiscard]] std::uint32_t scaleForCanvas(Size canvas) noexcept;

    [[nodiscard]] Theme scaledTheme(
        Theme base, std::uint32_t scale) noexcept;

}
