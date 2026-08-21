#include "antwika/ui/Theme.hpp"

#include <cstdint>

#include "Saturate.hpp"

namespace antwika::ui
{

    namespace
    {
        constexpr std::uint32_t kCanvasPerPixel = 240;

        std::uint32_t scaled(
            std::uint32_t value, std::uint32_t scale) noexcept
        {
            return detail::clampToU32(
                std::uint64_t{value} * std::uint64_t{scale});
        }
    }

    std::uint32_t scaleForCanvas(Size canvasSize) noexcept
    {
        const auto scale = canvasSize.height / kCanvasPerPixel;

        return scale > 0 ? scale : 1;
    }

    Theme scaledTheme(Theme baseTheme, std::uint32_t scale) noexcept
    {
        baseTheme.textScale = scaled(baseTheme.textScale, scale);
        baseTheme.padding = scaled(baseTheme.padding, scale);
        baseTheme.gap = scaled(baseTheme.gap, scale);
        baseTheme.buttonPadding = scaled(baseTheme.buttonPadding, scale);
        baseTheme.focusRingThickness = scaled(
            baseTheme.focusRingThickness,
            scale);
        baseTheme.scrollbarWidth = scaled(baseTheme.scrollbarWidth, scale);
        baseTheme.dividerThickness =
            scaled(baseTheme.dividerThickness, scale);

        return baseTheme;
    }

}
