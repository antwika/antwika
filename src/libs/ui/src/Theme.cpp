#include "antwika/ui/Theme.hpp"

#include <cstdint>

#include "Saturate.hpp"

namespace antwika::ui
{

    namespace
    {
        constexpr std::uint32_t kCanvasPerPixel = 240;

        std::uint32_t getScaledLength(
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
        baseTheme.textScale = getScaledLength(baseTheme.textScale, scale);
        baseTheme.padding = getScaledLength(baseTheme.padding, scale);
        baseTheme.gap = getScaledLength(baseTheme.gap, scale);
        baseTheme.buttonPadding = getScaledLength(baseTheme.buttonPadding, scale);
        baseTheme.focusRingThickness = getScaledLength(
            baseTheme.focusRingThickness,
            scale);
        baseTheme.scrollbarWidth = getScaledLength(baseTheme.scrollbarWidth, scale);
        baseTheme.dividerThickness =
            getScaledLength(baseTheme.dividerThickness, scale);

        return baseTheme;
    }

}
