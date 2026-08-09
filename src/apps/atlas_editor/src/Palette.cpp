#include "antwika/atlas_editor/Palette.hpp"

#include <array>
#include <span>

#include <antwika/gfx/Color.hpp>

namespace antwika::atlas_editor
{

    namespace
    {
        constexpr std::array kColors{
            Color{.red = 20, .green = 20, .blue = 24},
            Color{.red = 90, .green = 92, .blue = 98},
            Color{.red = 168, .green = 172, .blue = 176},
            Color{.red = 240, .green = 240, .blue = 236},
            Color{.red = 58, .green = 96, .blue = 52},
            Color{.red = 96, .green = 148, .blue = 76},
            Color{.red = 140, .green = 118, .blue = 82},
            Color{.red = 186, .green = 168, .blue = 128},
            Color{.red = 158, .green = 68, .blue = 54},
            Color{.red = 214, .green = 138, .blue = 66},
            Color{.red = 48, .green = 96, .blue = 152},
            Color{.red = 96, .green = 176, .blue = 208},
        };
    }

    std::span<const Color> defaultPalette() noexcept
    {
        return kColors;
    }

}
