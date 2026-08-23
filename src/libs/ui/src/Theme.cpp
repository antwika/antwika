#include "antwika/ui/Theme.hpp"

#include <cstdint>

#include "Saturate.hpp"

namespace antwika::ui
{

    namespace
    {
        constexpr std::uint32_t kCanvasPerPixel = 240;

        std::uint32_t getScaled(
            std::uint32_t value, std::uint32_t scale) noexcept
        {
            return detail::getClampToU32(
                std::uint64_t{value} * std::uint64_t{scale});
        }
    }

    std::uint32_t getScaleForCanvas(Size canvasSize) noexcept
    {
        const auto scale = canvasSize.height / kCanvasPerPixel;

        return scale > 0 ? scale : 1;
    }

    Theme getScaledTheme(Theme baseTheme, std::uint32_t scale) noexcept
    {
        baseTheme.textScale = getScaled(baseTheme.textScale, scale);
        baseTheme.padding = getScaled(baseTheme.padding, scale);
        baseTheme.gap = getScaled(baseTheme.gap, scale);
        baseTheme.buttonPadding = getScaled(baseTheme.buttonPadding, scale);
        baseTheme.focusRingThickness = getScaled(
            baseTheme.focusRingThickness,
            scale);
        baseTheme.scrollbarWidth = getScaled(baseTheme.scrollbarWidth, scale);
        baseTheme.dividerThickness =
            getScaled(baseTheme.dividerThickness, scale);

        return baseTheme;
    }

}
