#include "antwika/ui/Theme.hpp"

#include <cstdint>

#include "Saturate.hpp"

namespace antwika::ui
{

    namespace
    {
        // Canvas height each glyph pixel is worth.
        // Straight from apps/poker, where it already earned its keep.
        constexpr std::uint32_t kCanvasPerPixel = 240;

        std::uint32_t scaled(
            std::uint32_t value, std::uint32_t scale) noexcept
        {
            return detail::clampToU32(
                std::uint64_t{value} * std::uint64_t{scale});
        }
    } // namespace

    std::uint32_t scaleForCanvas(Size canvas) noexcept
    {
        const auto scale = canvas.height / kCanvasPerPixel;

        return scale > 0 ? scale : 1;
    }

    Theme scaledTheme(Theme base, std::uint32_t scale) noexcept
    {
        base.textScale = scaled(base.textScale, scale);
        base.padding = scaled(base.padding, scale);
        base.gap = scaled(base.gap, scale);
        base.buttonPadding = scaled(base.buttonPadding, scale);
        base.focusRingThickness = scaled(base.focusRingThickness, scale);

        return base;
    }

} // namespace antwika::ui
